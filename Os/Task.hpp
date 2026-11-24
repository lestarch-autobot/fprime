// ======================================================================
// \title Os/Task.hpp
// \brief common function definitions for Os::Task
// ======================================================================
#ifndef Os_Task_hpp_
#define Os_Task_hpp_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Time/TimeInterval.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Os/Mutex.hpp>
#include <Os/Os.hpp>
#include <Os/TaskString.hpp>
#include "config/OsSelection.hpp"  // defines the Os::Task alias and OS_TASK_HEADER; do not remove

#include <Fw/Deprecate.hpp>
#include <Fw/Types/Assert.hpp>
#include <limits>

// Forward declare for UTs
namespace Os {
namespace Test {
namespace Task {
struct Tester;
}
}  // namespace Test
}  // namespace Os

namespace Os {

// Forward declarations
template <typename T>
class TaskCommon;
template <typename K>
class TaskRegistryCommon;
//! Task registry for the Os::Task selected in config/OsSelection.hpp
using TaskRegistry = TaskRegistryCommon<Task>;

//! Task handle representation
class TaskHandle {};

//! \brief types and constants shared by users of Os::Task and by its implementations
struct TaskUserInterface {
    //! Sentinel value to use a default value for the task argument to which this is supplied.
    //!
    //! Implementations of TaskInterface::start() should make a special case to use some default value that is
    //! valid for the target platform.
    static constexpr FwSizeType TASK_DEFAULT = std::numeric_limits<FwSizeType>::max();

    //! Sentinel value to use a default priority for the task.
    //!
    //! Implementations of TaskInterface::start() should make a special case to use some default task priority
    //! that is valid for the target platform.
    static constexpr FwTaskPriorityType TASK_PRIORITY_DEFAULT = std::numeric_limits<FwTaskPriorityType>::max();

    enum Status {
        OP_OK,             //!< message sent/received okay
        INVALID_HANDLE,    //!< Task handle invalid
        INVALID_PARAMS,    //!< started task with invalid parameters
        INVALID_PRIORITY,  //!< started task with invalid priority
        INVALID_STACK,     //!< started with invalid stack size
        UNKNOWN_ERROR,     //!< unexpected error return value
        INVALID_AFFINITY,  //!< unable to set the task affinity
        DELAY_ERROR,       //!< error trying to delay the task
        JOIN_ERROR,        //!< error trying to join the task
        ERROR_RESOURCES,   //!< unable to allocate more tasks
        ERROR_PERMISSION,  //!< permissions error setting-up tasks
        NOT_SUPPORTED,     //!< Task feature is not supported
        INVALID_STATE,     //!< Task is in an invalid state for the operation
    };

    enum SuspensionType { INTENTIONAL, UNINTENTIONAL };

    enum State { NOT_STARTED, STARTING, RUNNING, SUSPENDED_INTENTIONALLY, SUSPENDED_UNINTENTIONALLY, EXITED, UNKNOWN };

    //! Prototype for task routine started in task context
    typedef void (*taskRoutine)(void* ptr);

    class Arguments {
      public:
        //! \brief construct a set of arguments to start a task
        //!
        //! Construct a set of arguments to start a task. It is illegal to supply a task routine that is
        //! set to a nullptr.
        //!
        //! \param name: name of the task
        //! \param routine: routine to run as part of this task
        //! \param routine_argument: (optional) argument to supply to the task routine
        //! \param priority: (optional) priority of this task
        //! \param stackSize: (optional) size of stack supplied to this task
        //! \param cpuAffinity: (optional) cpu affinity of this task. TODO: fix this into an array
        //! \param identifier: (optional) identifier for this task
        Arguments(const Fw::ConstStringBase& name,
                  const taskRoutine routine,
                  void* const routine_argument = nullptr,
                  const FwTaskPriorityType priority = TASK_PRIORITY_DEFAULT,
                  const FwSizeType stackSize = TASK_DEFAULT,
                  const FwSizeType cpuAffinity = TASK_DEFAULT,
                  const FwTaskIdType identifier = static_cast<FwTaskIdType>(TASK_DEFAULT))
            : m_name(name),
              m_routine(routine),
              m_routine_argument(routine_argument),
              m_priority(priority),
              m_stackSize(stackSize),
              m_cpuAffinity(cpuAffinity),
              m_identifier(identifier) {
            FW_ASSERT(routine != nullptr);
        }

      public:
        const Os::TaskString m_name;
        taskRoutine m_routine;
        void* m_routine_argument;
        FwTaskPriorityType m_priority;
        FwSizeType m_stackSize;
        FwSizeType m_cpuAffinity;
        FwTaskIdType m_identifier;
    };
};

//! Task class intended to be used by the rest of the fprime system. This is final as it is not intended to be a
//! parent class. Instead it wraps a delegate of the template type `T` to provide system specific
//! behaviour.
template <typename T>
class TaskCommon final : public TaskUserInterface {
    friend struct Os::Test::Task::Tester;

  public:
    //! Wrapper for task routine that ensures `onStart()` is called once the task actually begins
    class TaskRoutineWrapper {
      public:
        explicit TaskRoutineWrapper(TaskCommon& self);

        //! \brief run the task routine wrapper
        //!
        //! Sets the Os::Task to started via the setStarted method. Then runs the user function passing in the
        //! user argument.
        //! \param task_pointer: pointer to TaskRoutineWrapper being run
        static void run(void* task_pointer);

        //! \brief invoke the run method with "self" as argument
        void invoke();

        TaskCommon& m_task;                     //!< Reference to owning task
        taskRoutine m_user_function = nullptr;  //!< User function to run once started
        void* m_user_argument = nullptr;        //!<  Argument to user function
    };

    //! \brief backwards-compatible parameter type
    typedef FwSizeType ParamType;

    //! \brief default constructor
    TaskCommon();

    //! \brief default virtual destructor
    ~TaskCommon();

    //! \brief copy constructor is forbidden
    TaskCommon(const TaskCommon& other) = delete;

    //! \brief assignment operator is forbidden
    TaskCommon& operator=(const TaskCommon& other) = delete;

    //! \brief suspend the current task
    //!
    //! Suspend the current task unintentionally. If the user needs to indicate that the task was suspended
    //! intentionally then a call to `suspend(SuspensionType::INTENTIONAL)` should be used.
    void suspend();

    //! \brief get the task's state
    //!
    //! Returns the task state: not started, running, suspended (intentionally), suspended (unintentionally),
    //! and exited.
    //!
    //! \return task state
    State getState() const;

    //! \brief start this task
    //!
    //! Start this task with supplied name, task routine (run function), priority, stack, affinity, and task
    //! identifier. These arguments are supplied into an Arguments class and that version of the function is called.
    //! It is illegal to supply a nullptr as routine.
    //!
    //! \param name: name of the task to start
    //! \param routine: user routine to run
    //! \param arg: (optional) user argument to supply to task routine
    //! \param priority: (optional) priority of this task
    //! \param stackSize: (optional) stack size of this task
    //! \param cpuAffinity: (optional) affinity of this task. Use `Task::start(Arguments&)` to supply affinity set.
    //! \param identifier: (optional) identifier of this task
    //! \return: status of the start call
    DEPRECATED(Status start(const Fw::ConstStringBase& name,
                            const taskRoutine routine,
                            void* const arg = nullptr,
                            const FwTaskPriorityType priority = TASK_PRIORITY_DEFAULT,
                            const ParamType stackSize = TASK_DEFAULT,
                            const ParamType cpuAffinity = TASK_DEFAULT,
                            const ParamType identifier = TASK_DEFAULT),
               "Switch to Task::start(Arguments&)");

    //! \brief start the task
    //!
    //! Starts the task given the supplied arguments. This is done via a task routine wrapper intermediary that
    //! ensures that `setStarted` is called once the task has actually started to run. The task then runs the user
    //! routine. This function may return before the new task begins to run.
    //
    //! It is illegal for arguments.m_routine to be null.
    //!
    //! \param arguments: arguments supplied to the task start call
    //! \return status of the task start
    Status start(const Arguments& arguments);

    //! \brief perform delegate's required task start actions
    void onStart();

    //! \brief invoke the task's routine
    //~
    //! This will invoke the task's routine passing this as the argument to that call. This is used as a helper when
    //! running this task (e.g. repetitive cooperative calls).
    void invokeRoutine();

    //! \brief join calling thread to this thread
    //!
    //! Note: this function is deprecated as the value_ptr object is not used anyway and should always be set
    //! to nullptr.
    //!
    //! \param value_ptr must be set to nullptr
    //! \return status of the join
    DEPRECATED(Status join(void** value_ptr), "Please switch to argument free join.");

    //! \brief block until the task has ended
    //!
    //! Blocks the current (calling) task until this task execution has ended. Callers should ensure that any
    //! signals required to stop this task have already been emitted or will be emitted by another task.
    //!
    //! \return status of the block
    Status join();  //!< Wait for task to finish

    //! \brief suspend the task given the suspension type
    //!
    //! Suspends the task. Some implementations track if the suspension of a task was intentional or
    //! unintentional. The supplied `suspensionType` parameter indicates that this was intentional or
    //! unintentional. The type of suspension is also returned when calling `isSuspended`.
    //!
    //! \param suspensionType intentionality of the suspension
    void suspend(SuspensionType suspensionType);

    //! \brief resume a suspended task
    //!
    //! Resumes this task. Not started, running, and exited tasks take no action.
    //!
    void resume();

    //! \brief delay the current task
    //!
    //! Delays, or sleeps, the current task by the supplied time interval. In non-preempting os implementations
    //! the task will resume no earlier than expected but an exact wake-up time is not guaranteed.
    //!
    //! \param interval: delay time
    //! \return status of the delay
    Status _delay(const Fw::TimeInterval& interval);

    //! \brief determine if the task is cooperative multitasking (implementation specific)
    //! \return true if cooperative, false otherwise
    bool isCooperative();

    //! \brief get the task name
    TaskString getName();

    //! \brief get the task priority
    FwTaskPriorityType getPriority();

    //! \brief return the underlying task handle (implementation specific)
    //! \return internal task handle representation
    TaskHandle* getHandle();

    //! \brief initialize singleton
    static void init();

    //! \brief get the current number of tasks
    //! \return current number of tasks
    static FwSizeType getNumTasks();

    //! \brief register a task registry to track Threads
    //!
    static void registerTaskRegistry(TaskRegistryCommon<TaskCommon>* registry);

    //! \brief get a reference to singleton
    //! \return reference to singleton
    static TaskCommon& getSingleton();

    //! \brief delay the current task
    //!
    //! Delays, or sleeps, the current task by the supplied time interval. In non-preempting os implementations
    //! the task will resume no earlier than expected but an exact wake-up time is not guaranteed.
    //!
    //! \param interval: delay time
    //! \return status of the delay
    static Status delay(const Fw::TimeInterval& interval);

  private:
    static TaskRegistryCommon<TaskCommon>* s_taskRegistry;  //!< Pointer to registered task registry
    static FwSizeType s_numTasks;                           //!< Stores the number of tasks created.
    static Mutex s_taskMutex;                               //!< Guards s_numTasks

    TaskString m_name;  //!< Task object name
    TaskUserInterface::State m_state = TaskCommon::NOT_STARTED;
    mutable Mutex m_lock;               //!< Guards state transitions
    TaskRoutineWrapper m_wrapper;       //!< Concrete storage for task routine wrapper
    FwTaskPriorityType m_priority = 0;  // Storage of priority

    bool m_registered = false;  //!< Was this task registered

    // This section is used to store the implementation-defined file handle. To Os::File and fprime, this type is
    // opaque and thus normal allocation cannot be done. Instead, we allow the implementor to store then handle in
    // the byte-array here and set `handle` to that address for storage.
    //
    T m_delegate;  //!< Delegate for the real implementation
};

//! \brief task registry interface
//!
//! \tparam K: task type tracked by this registry. Os::TaskRegistry tracks Os::Task.
template <typename K>
class TaskRegistryCommon {
  public:
    //! \brief default task registry constructor
    TaskRegistryCommon() = default;
    //! \brief default task registry constructor
    virtual ~TaskRegistryCommon() = default;
    //! \brief add supplied task to the registry
    //!
    //! \param task: pointer to task to register
    virtual void addTask(K* task) = 0;  //!< Add a task to the registry

    //! \brief remove supplied task to the registry
    //!
    //! \param task: pointer to task to deregister
    virtual void removeTask(K* task) = 0;
};
}  // namespace Os

// Validate that OS_TASK_HEADER was defined by config/OsSelection.hpp
#ifndef OS_TASK_HEADER
#error "OS_TASK_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_TASK_HEADER

namespace Os {

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
TaskRegistryCommon<TaskCommon<T>>* TaskCommon<T>::s_taskRegistry = nullptr;
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
void TaskCommon<T>::registerTaskRegistry(TaskRegistryCommon<TaskCommon<T>>* registry) {
    TaskCommon::s_taskRegistry = registry;
}
}  // namespace Os

#endif
