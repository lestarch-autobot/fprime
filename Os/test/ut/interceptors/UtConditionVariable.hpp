// ======================================================================
// \title Os/test/ut/interceptors/UtConditionVariable.hpp
// \brief TEST ONLY: Os::ConditionVariable implementation selected in unit test builds
// ======================================================================
// Included before the guard: Os/Condition.hpp includes this file, and may be reached from the interface header
#include <Os/Interfaces/ConditionVariableInterface.hpp>
#ifndef OS_TEST_UT_INTERCEPTORS_UTCONDITIONVARIABLE_HPP_
#define OS_TEST_UT_INTERCEPTORS_UTCONDITIONVARIABLE_HPP_

#ifndef OS_SELECTED_CONDITION_VARIABLE_HEADER
#error "OS_SELECTED_CONDITION_VARIABLE_HEADER must be defined in config/OsSelection.hpp"
#endif
#include OS_SELECTED_CONDITION_VARIABLE_HEADER

namespace Os {
namespace Ut {
namespace Mutex {

//! \brief TEST ONLY: wraps the selected Os::ConditionVariable implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::ConditionVariableTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::ConditionVariable`) when no interceptor is installed.
class UtConditionVariable : public ConditionVariableInterface {
  public:
    UtConditionVariable();
    ~UtConditionVariable() override;

    //! \brief assignment operator is forbidden
    ConditionVariableInterface& operator=(const ConditionVariableInterface& other) override = delete;

    Status pend(Os::Mutex& mutex) override;
    void notify() override;
    void notifyAll() override;
    ConditionVariableHandle* getHandle() override;

  private:
    Os::Selected::ConditionVariable m_real;  //!< Implementation selected for non-test builds
};

}  // namespace Mutex
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_UTCONDITIONVARIABLE_HPP_
