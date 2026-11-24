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

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::Mutex in a unit test build
//!
//! A unit test subclasses this interface and overrides the functions it wants to intercept. Each function mirrors the
//! `MutexInterface` function of the same name with one extra first argument, `real`: the implementation a non-test
//! build would have used (e.g. `PosixMutex`) belonging to the `Os::Mutex` on which the call was made. An override may
//! call `real`, alter its results, or ignore it entirely. Functions that are not overridden pass through to `real`.
//! `constructed` and `destructed` report the lifetime of each `Os::Mutex` and do nothing unless overridden.
//!
//! The interceptor is active from construction to destruction, and at most one may exist at a time. While active it
//! receives the calls of every `Os::Mutex` in the process, from any thread, including mutexes constructed before it
//! was installed. Installation is not synchronized: construct the interceptor before starting tasks that use mutexes,
//! destroy it after they stop, and guard any state it shares between threads without using an `Os::Mutex`.
class MutexTestInterceptor : public MutexUserInterface {
  public:
    //! \brief constructor: installs this interceptor, asserting that no other interceptor is installed
    MutexTestInterceptor();

    //! \brief destructor: removes this interceptor
    virtual ~MutexTestInterceptor();

    MutexTestInterceptor(const MutexTestInterceptor& other) = delete;
    MutexTestInterceptor& operator=(const MutexTestInterceptor& other) = delete;

    //! \brief get the installed interceptor
    //! \return installed interceptor, or nullptr when none is installed
    static MutexTestInterceptor* getInstalled();

    //! \brief notification that an Os::Mutex finished constructing its implementation
    virtual void constructed(MutexInterface& real);

    //! \brief notification that an Os::Mutex is about to destroy its implementation
    virtual void destructed(MutexInterface& real);

    //! \brief intercept `MutexInterface::getHandle`
    virtual MutexHandle* getHandle(MutexInterface& real);

    //! \brief intercept `MutexInterface::take`
    virtual Status take(MutexInterface& real);

    //! \brief intercept `MutexInterface::release`
    virtual Status release(MutexInterface& real);

  private:
    static MutexTestInterceptor* s_installed;  //!< Installed interceptor, nullptr when none
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_MUTEXTESTINTERCEPTOR_HPP_
