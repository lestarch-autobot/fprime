// ======================================================================
// \title Os/test/ut/interceptors/TestInterceptor.hpp
// \brief TEST ONLY: installation of the Os test interceptors
//
// This header is for unit testing only. Do not use it in flight code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_TESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_TESTINTERCEPTOR_HPP_

#include <Fw/Types/Assert.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: installs the interceptor of one Os service for the lifetime of the object
//!
//! Base of each `Os::Test::<Service>TestInterceptor` (the `Interceptor` template argument). A unit test subclasses
//! the service's interceptor, overrides the functions it wants to intercept, and constructs one instance. The
//! `Os::Ut` implementation selected in unit test builds forwards every call made on that service to the installed
//! instance.
//!
//! The interceptor is active from construction to destruction, and at most one per service may exist at a time. While
//! active it receives the calls of every object of that service in the process, from any thread, including objects
//! constructed before it was installed. Installation is not synchronized: construct the interceptor before starting
//! tasks that use the service, destroy it after they stop, and guard any state it shares between threads without
//! using the intercepted service.
template <typename Interceptor>
class TestInterceptor {
  public:
    TestInterceptor(const TestInterceptor& other) = delete;
    TestInterceptor& operator=(const TestInterceptor& other) = delete;

    //! \brief get the installed interceptor
    //! \return installed interceptor, or nullptr when none is installed
    static Interceptor* getInstalled() { return s_installed; }

  protected:
    //! \brief constructor: installs this interceptor, asserting that no other interceptor is installed
    TestInterceptor() {
        FW_ASSERT(s_installed == nullptr);
        s_installed = static_cast<Interceptor*>(this);
    }

    //! \brief destructor: uninstalls this interceptor
    ~TestInterceptor() { s_installed = nullptr; }

  private:
    static Interceptor* s_installed;  //!< Installed interceptor, or nullptr
};

template <typename Interceptor>
Interceptor* TestInterceptor<Interceptor>::s_installed = nullptr;

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_TESTINTERCEPTOR_HPP_
