// ======================================================================
// \title Os/test/ut/interceptors/CountingSemaphoreTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::CountingSemaphore calls
//
// This interface is for unit testing only. It works with Os::Ut::CountingSemaphore::UtCountingSemaphore, the
// Os::CountingSemaphore implementation selected in unit test builds (BUILD_UT), and has no effect in any other build.
// Do not use it in flight code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_COUNTINGSEMAPHORETESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_COUNTINGSEMAPHORETESTINTERCEPTOR_HPP_

#include <Os/Interfaces/CountingSemaphoreInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::CountingSemaphore in a unit test build
//!
//! Each function mirrors the `CountingSemaphoreInterface` function of the same name with one extra first argument,
//! `real`: the implementation a non-test build would have used, belonging to the `Os::CountingSemaphore` on which the
//! call was made. Functions that are not overridden pass through to `real`; the lifetime notifications do nothing
//! unless overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class CountingSemaphoreTestInterceptor : public CountingSemaphoreUserInterface,
                                         public TestInterceptor<CountingSemaphoreTestInterceptor> {
  public:
    CountingSemaphoreTestInterceptor() = default;
    virtual ~CountingSemaphoreTestInterceptor() = default;

    //! \brief notification: an Os::CountingSemaphore implementation was constructed
    virtual void constructed(CountingSemaphoreInterface& real, U32 initial_count);

    //! \brief notification: an Os::CountingSemaphore implementation is being destroyed
    virtual void destructed(CountingSemaphoreInterface& real);

    //! \brief intercept `CountingSemaphoreInterface::wait`
    virtual Status wait(CountingSemaphoreInterface& real);

    //! \brief intercept `CountingSemaphoreInterface::waitTimeout`
    virtual Status waitTimeout(CountingSemaphoreInterface& real, const Fw::TimeInterval& interval);

    //! \brief intercept `CountingSemaphoreInterface::tryWait`
    virtual Status tryWait(CountingSemaphoreInterface& real);

    //! \brief intercept `CountingSemaphoreInterface::post`
    virtual Status post(CountingSemaphoreInterface& real);

    //! \brief intercept `CountingSemaphoreInterface::getHandle`
    virtual CountingSemaphoreHandle* getHandle(CountingSemaphoreInterface& real);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_COUNTINGSEMAPHORETESTINTERCEPTOR_HPP_
