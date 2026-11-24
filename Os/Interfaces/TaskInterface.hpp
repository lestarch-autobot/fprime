// ======================================================================
// \title Os/Interfaces/TaskInterface.hpp
// \brief pure-virtual interface implemented by Os::Task implementations
// ======================================================================
// Included before the guard: Os/Task.hpp includes the selected implementation, which includes this file
#include <Os/Task.hpp>

#ifndef OS_INTERFACES_TASKINTERFACE_HPP_
#define OS_INTERFACES_TASKINTERFACE_HPP_

namespace Os {

class TaskInterface : public TaskUserInterface {
  public:
    //! \brief default constructor
    TaskInterface() = default;

    //! \brief default virtual destructor
    virtual ~TaskInterface() = default;

    //! \brief copy constructor is forbidden
    TaskInterface(const TaskInterface& other) = delete;

    //! \brief assignment operator is forbidden
    TaskInterface& operator=(const TaskInterface& other) = delete;

    // =================
    // Implementation functions (static) to be supplied by the linker
    // =================

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
    //!
    //! \return true when the task expects cooperation, false otherwise
    virtual bool isCooperative() = 0;

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
};

}  // namespace Os

#endif  // OS_INTERFACES_TASKINTERFACE_HPP_
