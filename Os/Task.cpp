// ======================================================================
// \title Os/Task.cpp
// \brief common function implementation for Os::Task
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/Delegate.hpp>
#include <Os/Task.hpp>

namespace Os {

TaskUserInterface::Arguments::Arguments(const Fw::ConstStringBase& name,
                                        const Os::TaskUserInterface::taskRoutine routine,
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

template <typename T>
TaskCommon<T>::TaskRoutineWrapper::TaskRoutineWrapper(TaskCommon& self) : m_task(self) {}

template <typename T>
void TaskCommon<T>::TaskRoutineWrapper::run(void* wrapper_pointer) {
    FW_ASSERT(wrapper_pointer != nullptr);
    TaskRoutineWrapper& wrapper = *reinterpret_cast<TaskRoutineWrapper*>(wrapper_pointer);
    FW_ASSERT(wrapper.m_user_function != nullptr);

    wrapper.m_task.m_lock.lock();
    TaskCommon::State state = wrapper.m_task.m_state;
    wrapper.m_task.m_lock.unlock();
    FW_ASSERT(state != TaskCommon::State::NOT_STARTED);
    // Run once start code
    if (state == TaskCommon::State::STARTING) {
        wrapper.m_task.m_lock.lock();
        wrapper.m_task.m_state = TaskCommon::State::RUNNING;
        wrapper.m_task.m_lock.unlock();
        wrapper.m_task.onStart();
    }

    // Call user function supplying the user argument
    wrapper.m_user_function(wrapper.m_user_argument);
}

template <typename T>
void TaskCommon<T>::TaskRoutineWrapper::invoke() {
    TaskRoutineWrapper::run(this);
}

template <typename T>
TaskRegistry* TaskCommon<T>::s_taskRegistry = nullptr;
template <typename T>
FwSizeType TaskCommon<T>::s_numTasks = 0;
template <typename T>
Mutex TaskCommon<T>::s_taskMutex;

template <typename T>
TaskCommon<T>::TaskCommon() : m_wrapper(*this), m_delegate() {}

template <typename T>
TaskCommon<T>::~TaskCommon() {
    // If a registry has been registered and the task has been started then remove task from the registry
    if ((TaskCommon::s_taskRegistry != nullptr) && this->m_registered) {
        TaskCommon::s_taskRegistry->removeTask(this);
    }
}

template <typename T>
void TaskCommon<T>::suspend() {
    this->suspend(TaskCommon::SuspensionType::UNINTENTIONAL);
}

template <typename T>
typename TaskCommon<T>::State TaskCommon<T>::getState() const {
    TaskCommon::State state;
    this->m_lock.lock();
    state = this->m_state;
    this->m_lock.unlock();
    return state;
}

template <typename T>
typename TaskCommon<T>::Status TaskCommon<T>::start(const Fw::ConstStringBase& name,
                                                    const taskRoutine routine,
                                                    void* const arg,
                                                    const FwTaskPriorityType priority,
                                                    const ParamType stackSize,
                                                    const ParamType cpuAffinity,
                                                    const ParamType identifier) {
    FW_ASSERT(routine != nullptr);
    return this->start(TaskCommon::Arguments(name, routine, arg, priority, stackSize, cpuAffinity,
                                             static_cast<FwTaskIdType>(identifier)));
}

template <typename T>
typename TaskCommon<T>::Status TaskCommon<T>::start(const TaskCommon<T>::Arguments& arguments) {
    TaskCommon::init();
    // init call above is to ensure singleton is initialized in a thread-safe
    // manner and such that the address sanitizer does not inadvertently
    // result in a stack overflow when multiple calls to getSingleton are made
    // simultaneously from different threads. (As was observed in UT runs.)
    FW_ASSERT(arguments.m_routine != nullptr);
    this->m_name = arguments.m_name;
    this->m_state = State::STARTING;

    Arguments wrapped_arguments = arguments;
    // Intercept routine and argument with the local wrapper
    this->m_wrapper.m_user_function = arguments.m_routine;
    this->m_wrapper.m_user_argument = arguments.m_routine_argument;
    wrapped_arguments.m_routine = TaskCommon::TaskRoutineWrapper::run;
    wrapped_arguments.m_routine_argument = &this->m_wrapper;

    TaskCommon::Status status = this->m_delegate.start(wrapped_arguments);
    if (status == TaskCommon::Status::OP_OK) {
        TaskCommon::m_lock.lock();
        this->m_priority = wrapped_arguments.m_priority;
        TaskCommon::m_lock.unlock();
        TaskCommon::s_taskMutex.lock();
        TaskCommon::s_numTasks++;
        TaskCommon::s_taskMutex.unlock();

        // If a registry has been registered, register task to it
        if (TaskCommon::s_taskRegistry) {
            TaskCommon::s_taskRegistry->addTask(this);
            this->m_registered = true;
        }
    }
    return status;
}

template <typename T>
void TaskCommon<T>::onStart() {
    this->m_delegate.onStart();
}

template <typename T>
void TaskCommon<T>::invokeRoutine() {
    this->m_wrapper.invoke();
}

template <typename T>
typename TaskCommon<T>::Status TaskCommon<T>::join() {
    TaskCommon::Status status = TaskCommon::Status::INVALID_STATE;
    TaskCommon::State state = this->getState();
    if (state == TaskCommon::RUNNING || state == STARTING) {
        status = this->m_delegate.join();
        this->m_lock.lock();
        if (status == TaskCommon::Status::OP_OK) {
            this->m_state = TaskCommon::State::EXITED;
        } else {
            this->m_state = TaskCommon::State::UNKNOWN;
        }
        this->m_lock.unlock();
    }
    return status;
}

template <typename T>
void TaskCommon<T>::suspend(TaskCommon<T>::SuspensionType suspensionType) {
    this->m_delegate.suspend(suspensionType);
    this->m_lock.lock();
    this->m_state = (suspensionType == TaskCommon::SuspensionType::INTENTIONAL) ? State::SUSPENDED_INTENTIONALLY
                                                                                : State::SUSPENDED_UNINTENTIONALLY;
    this->m_lock.unlock();
}

template <typename T>
void TaskCommon<T>::resume() {
    this->m_delegate.resume();
}

template <typename T>
bool TaskCommon<T>::isCooperative() {
    return this->m_delegate.isCooperative();
}

template <typename T>
TaskString TaskCommon<T>::getName() {
    Os::ScopeLock lock(this->m_lock);
    return this->m_name;
}

template <typename T>
FwTaskPriorityType TaskCommon<T>::getPriority() {
    Os::ScopeLock lock(this->m_lock);
    return this->m_priority;
}

template <typename T>
TaskHandle* TaskCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

template <typename T>
FwSizeType TaskCommon<T>::getNumTasks() {
    TaskCommon::s_taskMutex.lock();
    FwSizeType num_tasks = TaskCommon::s_numTasks;
    TaskCommon::s_taskMutex.unlock();
    return num_tasks;
}

template <typename T>
Os::TaskUserInterface::Status TaskCommon<T>::_delay(const Fw::TimeInterval& interval) {
    return this->m_delegate._delay(interval);
}

template <typename T>
Os::TaskUserInterface::Status TaskCommon<T>::delay(const Fw::TimeInterval& interval) {
    return TaskCommon::getSingleton()._delay(interval);
}

template <typename T>
void TaskCommon<T>::init() {
    // Force trigger on the fly singleton setup
    (void)TaskCommon::getSingleton();
}

template <typename T>
TaskCommon<T>& TaskCommon<T>::getSingleton() {
    static TaskCommon s_singleton;
    return s_singleton;
}

template <typename T>
void TaskCommon<T>::registerTaskRegistry(TaskRegistry* registry) {
    TaskCommon::s_taskRegistry = registry;
}
// Instantiate the common class for the implementation selected in config/OsSelection.hpp
template class TaskCommon<Delegate::Selected<Task>::type>;

}  // namespace Os
