// ======================================================================
// \title Os/DelegateTask.hpp
// \brief Define the Os::DelegateTask class
// ======================================================================
#ifndef OS_DELEGATETASK_HPP_
#define OS_DELEGATETASK_HPP_

#include "Os/TaskInterface.hpp"

namespace Os {

//! \brief Link-time delegating Task implementation.
//!
//! Stores an implementation-defined task in a byte array and forwards all operations to a delegate
//! constructed (via placement-new) by TaskInterface::getDelegate(). Which getDelegate() is linked selects
//! the concrete implementation at link time. This is the default binding of the Os::Task alias; platforms
//! may instead alias Os::Task directly to a concrete implementation for compile-time selection (see
//! config/OsSelection.hpp).
class DelegateTask final : public TaskInterface {
  public:
    //! \brief default constructor
    DelegateTask();

    //! \brief default virtual destructor
    ~DelegateTask() final;

    //! \brief copy constructor is forbidden
    DelegateTask(const DelegateTask& other) = delete;

    //! \brief assignment operator is forbidden
    DelegateTask& operator=(const DelegateTask& other) = delete;

    // Keep the common TaskInterface overloads visible alongside the overrides below
    using TaskInterface::join;
    using TaskInterface::start;
    using TaskInterface::suspend;

    //! \brief start the task
    //!
    //! Starts the task given the supplied arguments. This is done via a task routine wrapper intermediary that
    //! ensures that `onStart` is called once the task has actually started to run. The task then runs the user
    //! routine. This function may return before the new task begins to run.
    //
    //! It is illegal for arguments.m_routine to be null.
    //!
    //! \param arguments: arguments supplied to the task start call
    //! \return status of the task start
    Status start(const Arguments& arguments) override;

    //! \brief perform delegate's required task start actions
    void onStart() override;

    //! \brief block until the task has ended
    //!
    //! \return status of the block
    Status join() override;

    //! \brief suspend the task given the suspension type
    //!
    //! \param suspensionType intentionality of the suspension
    void suspend(SuspensionType suspensionType) override;

    //! \brief resume a suspended task
    void resume() override;

    //! \brief delay the current task
    //!
    //! \param interval: delay time
    //! \return status of the delay
    Status _delay(const Fw::TimeInterval& interval) override;

    //! \brief determine if the task is cooperative multitasking (implementation specific)
    //! \return true if cooperative, false otherwise
    bool isCooperative() override;

    //! \brief return the underlying task handle (implementation specific)
    //! \return internal task handle representation
    TaskHandle* getHandle() override;

  private:
    // This section is used to store the implementation-defined task. To Os::Task and fprime, this type is
    // opaque and thus normal allocation cannot be done. Instead, we allow the implementor to store the task in
    // the byte-array here and set `m_delegate` to that address for storage.
    //
    alignas(FW_HANDLE_ALIGNMENT) TaskHandleStorage m_handle_storage;  //!< Storage for the aligned delegate
    TaskInterface& m_delegate;                                        //!< Delegate for the real implementation
};

}  // namespace Os

#endif  // OS_DELEGATETASK_HPP_
