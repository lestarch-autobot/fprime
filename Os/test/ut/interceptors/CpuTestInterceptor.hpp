// ======================================================================
// \title Os/test/ut/interceptors/CpuTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::Cpu calls
//
// This interface is for unit testing only. It works with Os::Ut::Cpu::UtCpu, the Os::Cpu implementation selected
// in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_CPUTESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_CPUTESTINTERCEPTOR_HPP_

#include <Os/Interfaces/CpuInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::Cpu in a unit test build
//!
//! Each function mirrors the `CpuInterface` function of the same name with one extra first argument, `real`: the
//! implementation a non-test build would have used, belonging to the `Os::Cpu` on which the call was made.
//! Functions that are not overridden pass through to `real`; the lifetime notifications do nothing unless
//! overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class CpuTestInterceptor : public CpuUserInterface, public TestInterceptor<CpuTestInterceptor> {
  public:
    CpuTestInterceptor() = default;
    virtual ~CpuTestInterceptor() = default;

    //! \brief notification: an Os::Cpu implementation was constructed
    virtual void constructed(CpuInterface& real);

    //! \brief notification: an Os::Cpu implementation is being destroyed
    virtual void destructed(CpuInterface& real);

    //! \brief intercept `CpuInterface::_getCount`
    virtual Status _getCount(CpuInterface& real, FwSizeType& cpu_count);

    //! \brief intercept `CpuInterface::_getTicks`
    virtual Status _getTicks(CpuInterface& real, Ticks& ticks, FwSizeType cpu_index);

    //! \brief intercept `CpuInterface::getHandle`
    virtual CpuHandle* getHandle(CpuInterface& real);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_CPUTESTINTERCEPTOR_HPP_
