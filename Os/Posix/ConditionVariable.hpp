// ======================================================================
// \title Os/Posix/ConditionVariable.hpp
// \brief Posix definitions for Os::ConditionVariable
// ======================================================================
#ifndef OS_POSIX_CONDITION_VARIABLE_HPP
#define OS_POSIX_CONDITION_VARIABLE_HPP
#include <pthread.h>
#include <Os/Condition.hpp>

namespace Os {
namespace Posix {
namespace Mutex {

struct PosixConditionVariableHandle : public ConditionVariableHandle {
    pthread_cond_t m_condition;  //! The condition variable
};

//! \brief Posix implementation of Os::ConditionVariable
//!
//! Posix implementation of `ConditionVariable` usable through the link-time delegate or selected directly as
//! `Os::ConditionVariable` (together with the matching `Os::Mutex`) in `config/OsSelection.hpp`.
//!
class PosixConditionVariable : public ConditionVariableInterface {
  public:
    //! \brief constructor
    //!
    PosixConditionVariable();

    //! \brief destructor
    //!
    ~PosixConditionVariable() override;

    ConditionVariableInterface& operator=(const ConditionVariableInterface& other) override = delete;

    //! \brief wait releasing mutex
    PosixConditionVariable::Status pend(Os::Mutex& mutex) override;

    //! \brief notify a single waiter
    void notify() override;

    //! \brief notify all current waiters
    void notifyAll() override;

    //! \brief get handle
    ConditionVariableHandle* getHandle() override;

  private:
    //! Handle for PosixMutex
    PosixConditionVariableHandle m_handle;
    //! Mutex first supplied to pend(); every later pend() must supply the same one
    Os::Mutex* m_lock = nullptr;
};

}  // namespace Mutex
}  // namespace Posix
}  // namespace Os
#endif  // OS_POSIX_CONDITION_VARIABLE_HPP
