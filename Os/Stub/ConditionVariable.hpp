// ======================================================================
// \title Os/Stub/ConditionVariable.hpp
// \brief Stub definitions for Os::ConditionVariable
// ======================================================================
#ifndef OS_STUB_CONDITION_VARIABLE_HPP
#define OS_STUB_CONDITION_VARIABLE_HPP
#include <Os/Condition.hpp>

namespace Os {
namespace Stub {
namespace Mutex {

struct StubConditionVariableHandle : public ConditionVariableHandle {};

//! \brief Stub implementation of Os::ConditionVariable
//!
//! Stub implementation of `ConditionVariable` usable through the link-time delegate or selected directly as
//! `Os::ConditionVariable` (together with any `Os::Mutex`) in `config/OsSelection.hpp`.
//!
class StubConditionVariable : public ConditionVariableInterface {
  public:
    //! \brief constructor
    //!
    StubConditionVariable() = default;

    //! \brief destructor
    //!
    ~StubConditionVariable() override = default;

    //! \brief assignment operator is forbidden
    ConditionVariableInterface& operator=(const ConditionVariableInterface& other) override = delete;

    //! \brief wait releasing mutex
    StubConditionVariable::Status pend(Os::Mutex& mutex) override;

    //! \brief notify a single waiter
    void notify() override;

    //! \brief notify all current waiters
    void notifyAll() override;

    //! \brief get handle
    ConditionVariableHandle* getHandle() override;

  private:
    //! Handle for PosixMutex
    StubConditionVariableHandle m_handle;
    //! Mutex first supplied to pend(); every later pend() must supply the same one
    Os::Mutex* m_lock = nullptr;
};

}  // namespace Mutex
}  // namespace Stub
}  // namespace Os
#endif  // OS_STUB_CONDITION_VARIABLE_HPP
