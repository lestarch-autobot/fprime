// ======================================================================
// \title Os/test/ut/interceptors/MutexTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::Mutex calls
//
// This interface is for unit testing only. It works with Os::Ut::Mutex::UtMutex, the Os::Mutex implementation selected
// in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_MUTEXTESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_MUTEXTESTINTERCEPTOR_HPP_

#include <Os/Interfaces/MutexInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::Mutex in a unit test build
//!
//! Each function mirrors the `MutexInterface` function of the same name with one extra first argument, `real`: the
//! implementation a non-test build would have used, belonging to the `Os::Mutex` on which the call was made.
//! Functions that are not overridden pass through to `real`; the lifetime notifications do nothing unless
//! overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class MutexTestInterceptor : public MutexUserInterface, public TestInterceptor<MutexTestInterceptor> {
  public:
    MutexTestInterceptor() = default;
    virtual ~MutexTestInterceptor() = default;

    //! \brief notification: an Os::Mutex implementation was constructed
    virtual void constructed(MutexInterface& real);

    //! \brief notification: an Os::Mutex implementation is being destroyed
    virtual void destructed(MutexInterface& real);

    //! \brief intercept `MutexInterface::getHandle`
    virtual MutexHandle* getHandle(MutexInterface& real);

    //! \brief intercept `MutexInterface::take`
    virtual Status take(MutexInterface& real);

    //! \brief intercept `MutexInterface::release`
    virtual Status release(MutexInterface& real);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_MUTEXTESTINTERCEPTOR_HPP_
