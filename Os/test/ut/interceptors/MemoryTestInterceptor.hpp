// ======================================================================
// \title Os/test/ut/interceptors/MemoryTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::Memory calls
//
// This interface is for unit testing only. It works with Os::Ut::Memory::UtMemory, the Os::Memory implementation
// selected in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_MEMORYTESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_MEMORYTESTINTERCEPTOR_HPP_

#include <Os/Interfaces/MemoryInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::Memory in a unit test build
//!
//! Each function mirrors the `MemoryInterface` function of the same name with one extra first argument, `real`: the
//! implementation a non-test build would have used, belonging to the `Os::Memory` on which the call was made.
//! Functions that are not overridden pass through to `real`; the lifetime notifications do nothing unless
//! overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class MemoryTestInterceptor : public MemoryUserInterface, public TestInterceptor<MemoryTestInterceptor> {
  public:
    MemoryTestInterceptor() = default;
    virtual ~MemoryTestInterceptor() = default;

    //! \brief notification: an Os::Memory implementation was constructed
    virtual void constructed(MemoryInterface& real);

    //! \brief notification: an Os::Memory implementation is being destroyed
    virtual void destructed(MemoryInterface& real);

    //! \brief intercept `MemoryInterface::_getUsage`
    virtual Status _getUsage(MemoryInterface& real, Usage& memory_usage);

    //! \brief intercept `MemoryInterface::getHandle`
    virtual MemoryHandle* getHandle(MemoryInterface& real);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_MEMORYTESTINTERCEPTOR_HPP_
