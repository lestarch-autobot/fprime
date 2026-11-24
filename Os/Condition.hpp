// ======================================================================
// \title Os/Condition.hpp
// \brief common definitions for Os::ConditionVariable
// ======================================================================
#ifndef OS_CONDITION_HPP_
#define OS_CONDITION_HPP_

#include <Os/Interfaces/ConditionVariableUserInterface.hpp>
#include "Fw/Types/Assert.hpp"
#include "Os/Mutex.hpp"
#include "Os/Os.hpp"
#include "config/ConditionVariableSelection.hpp"
#ifdef BUILD_UT
#include <Os/test/ut/implementations/UtConditionVariable.hpp>
#endif

namespace Os {

template <typename T>
class ConditionVariableCommon;

#ifdef BUILD_UT
//! \brief Os::ConditionVariable in unit test builds: wraps the implementation selected in
//! config/ConditionVariableSelection.hpp in the test-only UtConditionVariable, so that tests may intercept its calls
using ConditionVariable = ConditionVariableCommon<Ut::Mutex::UtConditionVariable>;
#else
//! \brief Os::ConditionVariable: the common class on the implementation selected in
//! config/ConditionVariableSelection.hpp
using ConditionVariable = ConditionVariableCommon<Selected::ConditionVariable>;
#endif

//! \brief Os::ConditionVariable wrapper around the implementation `T` selected at compile time
//!
//! Condition variables allow a program to block on a condition while atomically releasing an Os::Mutex and atomically
//! reacquiring the mutex once the condition has been notified. Holds the implementation by value and forwards every
//! operation to it. `Os::ConditionVariable` is this class instantiated on the implementation selected in
//! config/ConditionVariableSelection.hpp.
//!
//! \warning the implementation operates on the handle of the selected Os::Mutex: the Os::Mutex and
//! Os::ConditionVariable selections in config/ConditionVariableSelection.hpp must be compatible.
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
