// ======================================================================
// \title Os/test/ut/interceptors/ConditionVariableTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::ConditionVariable calls
//
// This interface is for unit testing only. It works with Os::Ut::Mutex::UtConditionVariable, the Os::ConditionVariable
// implementation selected in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight
// code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_CONDITIONVARIABLETESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_CONDITIONVARIABLETESTINTERCEPTOR_HPP_

#include <Os/Interfaces/ConditionVariableInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::ConditionVariable in a unit test build
//!
//! Each function mirrors the `ConditionVariableInterface` function of the same name with one extra first argument,
//! `real`: the implementation a non-test build would have used, belonging to the `Os::ConditionVariable` on which the
//! call was made. Functions that are not overridden pass through to `real`; the lifetime notifications do nothing
//! unless overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class ConditionVariableTestInterceptor : public ConditionVariableUserInterface,
                                         public TestInterceptor<ConditionVariableTestInterceptor> {
  public:
    ConditionVariableTestInterceptor() = default;
    virtual ~ConditionVariableTestInterceptor() = default;

    //! \brief notification: an Os::ConditionVariable implementation was constructed
    virtual void constructed(ConditionVariableInterface& real);

    //! \brief notification: an Os::ConditionVariable implementation is being destroyed
    virtual void destructed(ConditionVariableInterface& real);

    //! \brief intercept `ConditionVariableInterface::pend`
    virtual Status pend(ConditionVariableInterface& real, Os::Mutex& mutex);

    //! \brief intercept `ConditionVariableInterface::notify`
    virtual void notify(ConditionVariableInterface& real);

    //! \brief intercept `ConditionVariableInterface::notifyAll`
    virtual void notifyAll(ConditionVariableInterface& real);

    //! \brief intercept `ConditionVariableInterface::getHandle`
    virtual ConditionVariableHandle* getHandle(ConditionVariableInterface& real);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_CONDITIONVARIABLETESTINTERCEPTOR_HPP_
