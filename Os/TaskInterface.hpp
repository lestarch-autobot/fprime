// ======================================================================
// \title Os/TaskInterface.hpp
// \brief Os::TaskHandle, Os::TaskInterface, and Os::TaskRegistry definitions
// ======================================================================
#ifndef OS_TASKINTERFACE_HPP_
#define OS_TASKINTERFACE_HPP_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Time/TimeInterval.hpp>
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Os/Mutex.hpp>
#include <Os/Os.hpp>
#include <Os/TaskString.hpp>
#include "config/OsSelection.hpp"  // defines the Os::Task alias and OS_TASK_HEADER consumed by Os/Task.hpp

// Validate that config/OsSelection.hpp selects an Os::Task. Checked here rather than in Os/Task.hpp because this
// header itself uses the Os::Task alias. Only the macro can be checked: if `Task` is undeclared below,
// config/OsSelection.hpp defined OS_TASK_HEADER without the Os::Task alias.
#ifndef OS_TASK_HEADER
#error "config/OsSelection.hpp must select an Os::Task: define the Os::Task alias and OS_TASK_HEADER"
#endif

#include <Fw/Deprecate.hpp>
#include <limits>
#include <type_traits>

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
class TaskRegistry;

//! Task handle representation
class TaskHandle {};

class TaskInterface {
    friend struct Os::Test::Task::Tester;

  public:
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
                  const FwTaskIdType identifier = static_cast<FwTaskIdType>(TASK_DEFAULT));

      public:
        const Os::TaskString m_name;
        taskRoutine m_routine;
        void* m_routine_argument;
        FwTaskPriorityType m_priority;
        FwSizeType m_stackSize;
        FwSizeType m_cpuAffinity;
        FwTaskIdType m_identifier;
    };

    //! Wrapper for task routine that ensures `onStart()` is called once the task actually begins
    class TaskRoutineWrapper {
      public:
        explicit TaskRoutineWrapper(TaskInterface& self);

        //! \brief run the task routine wrapper
        //!
        //! Sets the Os::Task to started via the setStarted method. Then runs the user function passing in the
        //! user argument.
        //! \param task_pointer: pointer to TaskRoutineWrapper being run
        static void run(void* task_pointer);

        //! \brief invoke the run method with "self" as argument
        void invoke();

        TaskInterface& m_task;                  //!< Reference to owning task
        taskRoutine m_user_function = nullptr;  //!< User function to run once started
        void* m_user_argument = nullptr;        //!<  Argument to user function
    };

    //! \brief backwards-compatible parameter type
    typedef FwSizeType ParamType;

    //! \brief default constructor
    TaskInterface();

    //! \brief default virtual destructor
    virtual ~TaskInterface() = default;

    //! \brief copy constructor is forbidden
    TaskInterface(const TaskInterface& other) = delete;

    //! \brief assignment operator is forbidden
    TaskInterface& operator=(const TaskInterface& other) = delete;

    // =================
    // Implementation functions (static) to be supplied by the linker
    // =================

    //! \brief provide a pointer to a task delegate object
    //!
    //! This function must return a pointer to a `TaskInterface` object that contains the real implementation of the
    //! file functions as defined by the implementor.  This function must do several things to be considered correctly
    //! implemented:
    //!
    //! 1. Assert that the supplied memory is non-null. e.g `FW_ASSERT(aligned_placement_new_memory != NULL);`
    //! 2. Assert that their implementation fits within FW_TASK_HANDLE_MAX_SIZE.
    //!    e.g. `static_assert(sizeof(PosixTaskImplementation) <= sizeof(Os::TaskHandleStorage),
    //!        "FW_TASK_HANDLE_MAX_SIZE too small");`
    //! 3. Assert that their implementation aligns within FW_HANDLE_ALIGNMENT.
    //!    e.g. `static_assert((FW_HANDLE_ALIGNMENT % alignof(PosixTaskImplementation)) == 0, "Bad handle alignment");`
    //! 4. Placement new their implementation into `aligned_placement_new_memory`
    //!    e.g. `TaskInterface* interface = new (aligned_placement_new_memory) PosixTaskImplementation;`
    //! 5. Return the result of the placement new
    //!    e.g. `return interface;`
    //!
    //! \return result of placement new, must be equivalent to `aligned_placement_new_memory`
    //!
    static TaskInterface* getDelegate(TaskHandleStorage& aligned_placement_new_memory);

    // =================
    // Implementation functions (instance) to be supplied by the Os::TaskInterface children
    // =================

    //! \brief perform required task start actions
    virtual void onStart() = 0;

    //! \brief block until the task has ended
    //!
    //! Blocks the current (calling) task until this task execution has ended. Callers should ensure that any
    //! signals required to stop this task have already been emitted or will be emitted by another task.
    //!
    //! \return status of the block
    virtual Status join() = 0;

    //! \brief suspend the task given the suspension type
    //!
    //! Suspends the task. Some implementations track if the suspension of a task was intentional or
    //! unintentional. The supplied `suspensionType` parameter indicates that this was intentional or
    //! unintentional. The type of suspension is also returned when calling `isSuspended`.
    //!
    //! \param suspensionType intentionality of the suspension
    virtual void suspend(SuspensionType suspensionType) = 0;

    //! \brief resume a suspended task
    //!
    //! Resumes this task. Not started, running, and exited tasks take no action.
    //!
    virtual void resume() = 0;

    //! \brief delay the currently scheduled task using the given architecture
    //!
    //! Delays, or sleeps, the current task by the supplied time interval. In non-preempting os implementations
    //! the task will resume no earlier than expected but an exact wake-up time is not guaranteed.
    //!
    //! \param interval: delay time
    //! \return status of the delay
    virtual Status _delay(const Fw::TimeInterval& interval) = 0;

    //! \brief determine if the task requires cooperative multitasking
    //!
    //! Some task implementations require cooperative multitasking where the task execution is run by a user
    //! defined task scheduler and not the operating system task scheduler. These tasks cooperatively on
    //! multitask by doing one unit of work and return from the function.
    //!
    //! This function indicates if the task requires cooperative support.
    //! The default implementation returns false.
    //!
    //! \return true when the task expects cooperation, false otherwise
    virtual bool isCooperative();

    //! \brief return the underlying task handle (implementation specific)
    //! \return internal task handle representation
    virtual TaskHandle* getHandle() = 0;

    //! \brief start the task
    //!
    //! Starts the task given the supplied arguments.
    //!
    //! \param arguments: arguments supplied to the task start call
    //! \return status of the task start
    virtual Status start(const Arguments& arguments) = 0;

    // ------------------------------------------------------------------
    // Common (non-virtual) functions. Defined on the interface so they are available regardless of which
    // implementation Os::Task is configured to be.
    // ------------------------------------------------------------------

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

    //! \brief get the task name
    TaskString getName();

    //! \brief get the task priority
    FwTaskPriorityType getPriority();

    //! \brief initialize singleton
    static void init();

    //! \brief get the current number of tasks
    //! \return current number of tasks
    static FwSizeType getNumTasks();

    //! \brief register a task registry to track Threads
    //!
    static void registerTaskRegistry(TaskRegistry* registry);

    //! \brief get a reference to singleton
    //! \return reference to singleton
    static Task& getSingleton();

    //! \brief delay the current task
    //!
    //! Delays, or sleeps, the current task by the supplied time interval. In non-preempting os implementations
    //! the task will resume no earlier than expected but an exact wake-up time is not guaranteed.
    //!
    //! \param interval: delay time
    //! \return status of the delay
    static Status delay(const Fw::TimeInterval& interval);

  protected:
    // ------------------------------------------------------------------
    // Common bookkeeping helpers. Every implementation of start(), join(), suspend(), and the destructor must
    // call these, passing `this`, so that it provides the complete Os::Task behavior when it is selected
    // directly as the Os::Task alias (compile-time selection, see config/OsSelection.hpp).
    //
    // The bookkeeping is performed only when the calling class is the configured Os::Task. When the
    // implementation is instead owned by an Os::DelegateTask the bookkeeping belongs to the DelegateTask and
    // the helpers pass through without effect. The configured Os::Task must be the class that calls the
    // helpers: aliasing Os::Task to a class derived from an implementation is rejected at compile time.
    // The source that calls the helpers must include Os/Task.hpp so that Os::Task is a complete type.
    // ------------------------------------------------------------------

    //! \brief common actions required before an implementation starts a task
    //!
    //! Asserts that the routine is valid. For the configured Os::Task it also records the task name, moves the
    //! task to STARTING, and wraps the user routine and argument in this task's TaskRoutineWrapper so that the
    //! RUNNING transition and onStart() happen once the task begins to run. The implementation must start the
    //! task from the returned arguments.
    //!
    //! \param self: `this` of the calling implementation
    //! \param arguments: arguments supplied to the task start call
    //! \return arguments the implementation must use to start the task
    template <class Impl>
    Arguments prepareStart(Impl* self, const Arguments& arguments) {
        FW_ASSERT(arguments.m_routine != nullptr);
        return (TaskInterface::asTask(self) != nullptr) ? this->commonPrepareStart(arguments) : arguments;
    }

    //! \brief common actions required after an implementation attempted to start a task
    //!
    //! On success records the priority, counts the task, and adds it to the task registry (when registered).
    //!
    //! \param self: `this` of the calling implementation
    //! \param status: status of the implementation's start attempt
    //! \param arguments: arguments returned by prepareStart
    template <class Impl>
    void completeStart(Impl* self, Status status, const Arguments& arguments) {
        Task* const task = TaskInterface::asTask(self);
        if (task != nullptr) {
            this->commonCompleteStart(task, status, arguments);
        }
    }

    //! \brief check that the task is in a state that may be joined
    //!
    //! \param self: `this` of the calling implementation
    //! \return true when the implementation may join, false when it must return INVALID_STATE
    template <class Impl>
    bool prepareJoin(const Impl* self) const {
        return (TaskInterface::asTask(self) != nullptr) ? this->commonPrepareJoin() : true;
    }

    //! \brief record the outcome of an implementation's join
    //!
    //! \param self: `this` of the calling implementation
    //! \param status: status of the implementation's join
    template <class Impl>
    void completeJoin(Impl* self, Status status) {
        if (TaskInterface::asTask(self) != nullptr) {
            this->commonCompleteJoin(status);
        }
    }

    //! \brief record that the task was suspended
    //!
    //! \param self: `this` of the calling implementation
    //! \param suspensionType intentionality of the suspension
    template <class Impl>
    void completeSuspend(Impl* self, SuspensionType suspensionType) {
        if (TaskInterface::asTask(self) != nullptr) {
            this->commonCompleteSuspend(suspensionType);
        }
    }

    //! \brief remove the task from the task registry if it was added
    //!
    //! Must be called from the destructor of every implementation.
    //!
    //! \param self: `this` of the calling implementation
    template <class Impl>
    void deregister(Impl* self) {
        Task* const task = TaskInterface::asTask(self);
        if (task != nullptr) {
            this->commonDeregister(task);
        }
    }

  private:
    //! \brief convert the calling implementation to the configured Os::Task
    //!
    //! \param self: `this` of the calling implementation
    //! \return self when the calling class is the configured Os::Task, nullptr otherwise
    template <class Impl>
    static Task* asTask(Impl* self) {
        // Os::Task must be a complete type here: the source calling the helpers must include Os/Task.hpp
        // An Os::Task derived from an implementation would silently skip the bookkeeping
        static_assert(std::is_same<typename std::remove_cv<Impl>::type, Task>::value ||
                          (not std::is_base_of<typename std::remove_cv<Impl>::type, Task>::value),
                      "Os::Task must be the class that calls the TaskInterface helpers, not a class derived from it");
        FW_ASSERT(self != nullptr);
        // void* round trip is exact: it is only converted back to the type it came from
        return std::is_same<typename std::remove_cv<Impl>::type, Task>::value
                   ? static_cast<Task*>(const_cast<void*>(static_cast<const void*>(self)))
                   : nullptr;
    }

    Arguments commonPrepareStart(const Arguments& arguments);
    void commonCompleteStart(Task* task, Status status, const Arguments& arguments);
    bool commonPrepareJoin() const;
    void commonCompleteJoin(Status status);
    void commonCompleteSuspend(SuspensionType suspensionType);
    void commonDeregister(Task* task);

    static TaskRegistry* s_taskRegistry;  //!< Pointer to registered task registry
    static FwSizeType s_numTasks;         //!< Stores the number of tasks created.
    static Mutex s_taskMutex;             //!< Guards s_numTasks

    TaskString m_name;  //!< Task object name
    State m_state = NOT_STARTED;
    mutable Mutex m_lock;               //!< Guards state transitions
    TaskRoutineWrapper m_wrapper;       //!< Concrete storage for task routine wrapper
    FwTaskPriorityType m_priority = 0;  // Storage of priority

    bool m_registered = false;  //!< Was this task registered
};

class TaskRegistry {
  public:
    //! \brief default task registry constructor
    TaskRegistry() = default;
    //! \brief default task registry constructor
    virtual ~TaskRegistry() = default;
    //! \brief add supplied task to the registry
    //!
    //! \param task: pointer to task to register
    virtual void addTask(Task* task) = 0;  //!< Add a task to the registry

    //! \brief remove supplied task to the registry
    //!
    //! \param task: pointer to task to deregister
    virtual void removeTask(Task* task) = 0;
};
}  // namespace Os

#endif  // OS_TASKINTERFACE_HPP_
