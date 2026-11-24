// ======================================================================
// \title Os/Interfaces/TaskUserInterface.hpp
// \brief types shared by Os::Task users and implementations
//
// Includes no other Os service header, so both Os/Task.hpp and the Os::Task implementations may include it.
// ======================================================================
#ifndef OS_INTERFACES_TASKUSERINTERFACE_HPP_
#define OS_INTERFACES_TASKUSERINTERFACE_HPP_

#include <Fw/Deprecate.hpp>
#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Time/TimeInterval.hpp>
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Os/Os.hpp>
#include <Os/TaskString.hpp>
#include <limits>

namespace Os {

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

}  // namespace Os

#endif  // OS_INTERFACES_TASKUSERINTERFACE_HPP_
