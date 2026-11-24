// ======================================================================
// \title Os/test/ut/interceptors/ConsoleTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::Console calls
//
// This interface is for unit testing only. It works with Os::Ut::Console::UtConsole, the Os::Console implementation
// selected in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_CONSOLETESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_CONSOLETESTINTERCEPTOR_HPP_

#include <Os/Interfaces/ConsoleInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::Console in a unit test build
//!
//! Each function mirrors the `ConsoleInterface` function of the same name with one extra first argument, `real`: the
//! implementation a non-test build would have used, belonging to the `Os::Console` on which the call was made.
//! Functions that are not overridden pass through to `real`; the lifetime notifications do nothing unless
//! overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class ConsoleTestInterceptor : public ConsoleUserInterface, public TestInterceptor<ConsoleTestInterceptor> {
  public:
    ConsoleTestInterceptor() = default;
    virtual ~ConsoleTestInterceptor() = default;

    //! \brief notification: an Os::Console implementation was constructed
    virtual void constructed(ConsoleInterface& real);

    //! \brief notification: an Os::Console implementation is being destroyed
    virtual void destructed(ConsoleInterface& real);

    //! \brief notification: `real` was copy constructed from `other`
    virtual void copyConstructed(ConsoleInterface& real, const ConsoleInterface& other);

    //! \brief notification: `other` was copy assigned to `real`
    virtual void copyAssigned(ConsoleInterface& real, const ConsoleInterface& other);

    //! \brief intercept `ConsoleInterface::writeMessage`
    virtual void writeMessage(ConsoleInterface& real, const CHAR* message, const FwSizeType size);

    //! \brief intercept `ConsoleInterface::getHandle`
    virtual ConsoleHandle* getHandle(ConsoleInterface& real);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_CONSOLETESTINTERCEPTOR_HPP_
