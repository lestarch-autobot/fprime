// ======================================================================
// \title Os/TaskInterface.cpp
// \brief common function implementation for Os::TaskInterface
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/Task.hpp>

namespace Os {

TaskInterface::Arguments::Arguments(const Fw::ConstStringBase& name,
                                    const Os::TaskInterface::taskRoutine routine,
                                    void* const routine_argument,
                                    const FwTaskPriorityType priority,
                                    const FwSizeType stackSize,
                                    const FwSizeType cpuAffinity,
                                    const FwTaskIdType identifier)
    : m_name(name),
      m_routine(routine),
      m_routine_argument(routine_argument),
      m_priority(priority),
      m_stackSize(stackSize),
      m_cpuAffinity(cpuAffinity),
      m_identifier(identifier) {
    FW_ASSERT(routine != nullptr);
}

TaskInterface::TaskRoutineWrapper::TaskRoutineWrapper(TaskInterface& self) : m_task(self) {}

void TaskInterface::TaskRoutineWrapper::run(void* wrapper_pointer) {
    FW_ASSERT(wrapper_pointer != nullptr);
    TaskRoutineWrapper& wrapper = *reinterpret_cast<TaskRoutineWrapper*>(wrapper_pointer);
    FW_ASSERT(wrapper.m_user_function != nullptr);

    wrapper.m_task.m_lock.lock();
    State state = wrapper.m_task.m_state;
    wrapper.m_task.m_lock.unlock();
    FW_ASSERT(state != State::NOT_STARTED);
    // Run once start code
    if (state == State::STARTING) {
        wrapper.m_task.m_lock.lock();
        wrapper.m_task.m_state = State::RUNNING;
        wrapper.m_task.m_lock.unlock();
        wrapper.m_task.onStart();
    }

    // Call user function supplying the user argument
    wrapper.m_user_function(wrapper.m_user_argument);
}

void TaskInterface::TaskRoutineWrapper::invoke() {
    TaskRoutineWrapper::run(this);
}

TaskRegistry* TaskInterface::s_taskRegistry = nullptr;
FwSizeType TaskInterface::s_numTasks = 0;
Mutex TaskInterface::s_taskMutex;

TaskInterface::TaskInterface() : m_wrapper(*this) {}

bool TaskInterface::isCooperative() {
    return false;
}

void TaskInterface::suspend() {
    this->suspend(SuspensionType::UNINTENTIONAL);
}

TaskInterface::State TaskInterface::getState() const {
    State state;
    this->m_lock.lock();
    state = this->m_state;
    this->m_lock.unlock();
    return state;
}

TaskInterface::Status TaskInterface::start(const Fw::ConstStringBase& name,
                                           const taskRoutine routine,
                                           void* const arg,
                                           const FwTaskPriorityType priority,
                                           const ParamType stackSize,
                                           const ParamType cpuAffinity,
                                           const ParamType identifier) {
    FW_ASSERT(routine != nullptr);
    return this->start(
        Arguments(name, routine, arg, priority, stackSize, cpuAffinity, static_cast<FwTaskIdType>(identifier)));
}

void TaskInterface::invokeRoutine() {
    this->m_wrapper.invoke();
}

TaskString TaskInterface::getName() {
    Os::ScopeLock lock(this->m_lock);
    return this->m_name;
}

FwTaskPriorityType TaskInterface::getPriority() {
    Os::ScopeLock lock(this->m_lock);
    return this->m_priority;
}

FwSizeType TaskInterface::getNumTasks() {
    TaskInterface::s_taskMutex.lock();
    FwSizeType num_tasks = TaskInterface::s_numTasks;
    TaskInterface::s_taskMutex.unlock();
    return num_tasks;
}

TaskInterface::Status TaskInterface::delay(const Fw::TimeInterval& interval) {
    return TaskInterface::getSingleton()._delay(interval);
}

void TaskInterface::init() {
    // Force trigger on the fly singleton setup
    (void)TaskInterface::getSingleton();
}

Task& TaskInterface::getSingleton() {
    static Task s_singleton;
    return s_singleton;
}

void TaskInterface::registerTaskRegistry(TaskRegistry* registry) {
    TaskInterface::s_taskRegistry = registry;
}

TaskInterface::Arguments TaskInterface::commonPrepareStart(const Arguments& arguments) {
    TaskInterface::init();
    // init call above is to ensure singleton is initialized in a thread-safe
    // manner and such that the address sanitizer does not inadvertently
    // result in a stack overflow when multiple calls to getSingleton are made
    // simultaneously from different threads. (As was observed in UT runs.)
    this->m_name = arguments.m_name;
    this->m_state = State::STARTING;

    Arguments wrapped_arguments = arguments;
    // Intercept routine and argument with the local wrapper
    this->m_wrapper.m_user_function = arguments.m_routine;
    this->m_wrapper.m_user_argument = arguments.m_routine_argument;
    wrapped_arguments.m_routine = TaskRoutineWrapper::run;
    wrapped_arguments.m_routine_argument = &this->m_wrapper;
    return wrapped_arguments;
}

void TaskInterface::commonCompleteStart(Task* task, Status status, const Arguments& arguments) {
    if (status == Status::OP_OK) {
        this->m_lock.lock();
        this->m_priority = arguments.m_priority;
        this->m_lock.unlock();
        TaskInterface::s_taskMutex.lock();
        TaskInterface::s_numTasks++;
        TaskInterface::s_taskMutex.unlock();

        // If a registry has been registered, register task to it
        if (TaskInterface::s_taskRegistry != nullptr) {
            TaskInterface::s_taskRegistry->addTask(task);
            this->m_registered = true;
        }
    }
}

bool TaskInterface::commonPrepareJoin() const {
    const State state = this->getState();
    return (state == State::RUNNING) || (state == State::STARTING);
}

void TaskInterface::commonCompleteJoin(Status status) {
    this->m_lock.lock();
    this->m_state = (status == Status::OP_OK) ? State::EXITED : State::UNKNOWN;
    this->m_lock.unlock();
}

void TaskInterface::commonCompleteSuspend(SuspensionType suspensionType) {
    this->m_lock.lock();
    this->m_state = (suspensionType == SuspensionType::INTENTIONAL) ? State::SUSPENDED_INTENTIONALLY
                                                                    : State::SUSPENDED_UNINTENTIONALLY;
    this->m_lock.unlock();
}

void TaskInterface::commonDeregister(Task* task) {
    // If a registry has been registered and the task has been started then remove task from the registry
    if ((TaskInterface::s_taskRegistry != nullptr) && this->m_registered) {
        TaskInterface::s_taskRegistry->removeTask(task);
        this->m_registered = false;
    }
}
}  // namespace Os
