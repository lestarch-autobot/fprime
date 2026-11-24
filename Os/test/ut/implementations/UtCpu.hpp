// ======================================================================
// \title Os/test/ut/implementations/UtCpu.hpp
// \brief TEST ONLY: Os::Cpu implementation selected in unit test builds
// ======================================================================
// Included before the guard: Os/Cpu.hpp includes this file, and may be reached from the interface header
#include <Os/Interfaces/CpuInterface.hpp>
#ifndef OS_TEST_UT_IMPLEMENTATIONS_UTCPU_HPP_
#define OS_TEST_UT_IMPLEMENTATIONS_UTCPU_HPP_

#ifndef OS_SELECTED_CPU_HEADER
#error "OS_SELECTED_CPU_HEADER must be defined in config/OsSelection.hpp"
#endif
#include OS_SELECTED_CPU_HEADER

namespace Os {
namespace Ut {
namespace Cpu {

//! \brief TEST ONLY: wraps the selected Os::Cpu implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::CpuTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::Cpu`) when no interceptor is installed.
class UtCpu : public CpuInterface {
  public:
    UtCpu();
    ~UtCpu() override;

    //! \brief assignment operator is forbidden
    CpuInterface& operator=(const CpuInterface& other) override = delete;

    Status _getCount(FwSizeType& cpu_count) override;
    Status _getTicks(Ticks& ticks, FwSizeType cpu_index) override;
    CpuHandle* getHandle() override;

  private:
    Os::Selected::Cpu m_real;  //!< Implementation selected for non-test builds
};

}  // namespace Cpu
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_IMPLEMENTATIONS_UTCPU_HPP_
