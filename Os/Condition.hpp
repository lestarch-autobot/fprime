// ======================================================================
// \title Os/Condition.hpp
// \brief common definitions for Os::ConditionVariable
// ======================================================================
#ifndef OS_CONDITION_HPP_
#define OS_CONDITION_HPP_

#include "Fw/Types/Assert.hpp"
#include "Os/Mutex.hpp"
#include "Os/Os.hpp"
#include "config/OsSelection.hpp"  // defines the Os::ConditionVariable alias and OS_CONDITION_VARIABLE_HEADER; do not remove

namespace Os {

//! \brief Condition variable handle parent
class ConditionVariableHandle {};

//! \brief types shared by Os::ConditionVariable users and implementations
struct ConditionVariableUserInterface {
    enum Status {
        OP_OK,                  //!<  Operation was successful
        ERROR_MUTEX_NOT_HELD,   //!< When trying to wait but we don't hold the mutex
        ERROR_DIFFERENT_MUTEX,  //!< When trying to use a different mutex than expected mutex
        ERROR_NOT_IMPLEMENTED,  //!< When trying to use a feature that isn't implemented
        NOT_SUPPORTED,          //!< ConditionVariable does not support operation
        ERROR_OTHER             //!< All other errors
    };
};

//! \brief Os::ConditionVariable wrapper around the implementation `T` selected at compile time
//!
//! Condition variables allow a program to block on a condition while atomically releasing an Os::Mutex and atomically
//! reacquiring the mutex once the condition has been notified. Holds the implementation by value and forwards every
//! operation to it. `Os::ConditionVariable` is this class instantiated on the implementation selected in
//! config/OsSelection.hpp.
//!
//! \warning the implementation operates on the handle of the selected Os::Mutex: the Os::Mutex and
//! Os::ConditionVariable selections in config/OsSelection.hpp must be compatible.
template <typename T>
class ConditionVariableCommon final : public ConditionVariableUserInterface {
  public:
    //! \brief default constructor
    ConditionVariableCommon() = default;

    //! \brief default destructor
    ~ConditionVariableCommon() = default;

    //! \brief copy constructor is forbidden
    ConditionVariableCommon(const ConditionVariableCommon& other) = delete;

    //! \brief assignment operator is forbidden
    ConditionVariableCommon& operator=(const ConditionVariableCommon& other) = delete;

    //! \brief wait on a condition variable
    //!
    //! Wait on a condition variable. This function will atomically unlock the provided mutex and block on the condition
    //! in one step. Blocking will occur until a future `notify` or `notifyAll` call is made to this variable on another
    //! thread of execution. The mutex first supplied is recorded: supplying a different mutex later returns
    //! ERROR_DIFFERENT_MUTEX.
    //!
    //! \param mutex: mutex to unlock as part of this operation
    //! \return status of the conditional wait
    Status pend(Os::Mutex& mutex);

    //! \brief wait on a condition variable and assert success
    //!
    //! \warning it is invalid to supply a mutex different from those supplied by others
    //! \warning conditions *must* be rechecked after the condition variable unlocks
    //! \warning the mutex must be locked by the calling task
    //!
    //! \param mutex: mutex to unlock as part of this operation
    void wait(Os::Mutex& mutex);

    //! \brief notify a single waiter on this condition variable
    //!
    //! Notify a single waiter on this condition variable. It is not necessary to hold the mutex supplied by the waiters
    //! and it is advantageous not to hold the lock to prevent immediate re-blocking.
    void notify();

    //! \brief notify all waiters on this condition variable
    //!
    //! Notify all waiters on this condition variable. It is not necessary to hold the mutex supplied by the waiters
    //! and it is advantageous not to hold the lock to prevent immediate re-blocking.
    void notifyAll();

    //! \brief return the underlying condition variable handle (implementation specific).
    //! \return internal condition variable handle representation
    ConditionVariableHandle* getHandle();

  private:
    Os::Mutex* m_lock = nullptr;  //!< Mutex first supplied to pend
    T m_delegate;                 //!< Delegate for the real implementation
};

}  // namespace Os

// Validate that OS_CONDITION_VARIABLE_HEADER was defined by config/OsSelection.hpp
#ifndef OS_CONDITION_VARIABLE_HEADER
#error "OS_CONDITION_VARIABLE_HEADER must be defined in config/OsSelection.hpp, paired with Os::Mutex"
#endif

#include OS_CONDITION_VARIABLE_HEADER

namespace Os {

template <typename T>
typename ConditionVariableCommon<T>::Status ConditionVariableCommon<T>::pend(Os::Mutex& mutex) {
    if (this->m_lock != nullptr && this->m_lock != &mutex) {
        return Status::ERROR_DIFFERENT_MUTEX;
    }
    this->m_lock = &mutex;
    return this->m_delegate.pend(mutex);
}

template <typename T>
void ConditionVariableCommon<T>::wait(Os::Mutex& mutex) {
    const Status status = this->pend(mutex);
    FW_ASSERT(status == Status::OP_OK, static_cast<FwAssertArgType>(reinterpret_cast<PlatformPointerCastType>(this)),
              status);
}

template <typename T>
void ConditionVariableCommon<T>::notify() {
    this->m_delegate.notify();
}

template <typename T>
void ConditionVariableCommon<T>::notifyAll() {
    this->m_delegate.notifyAll();
}

template <typename T>
ConditionVariableHandle* ConditionVariableCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

}  // namespace Os

#endif  // OS_CONDITION_HPP_
