// ======================================================================
// \title config/CpuSelection.hpp
// \brief configured selection of the Os::Cpu implementation
//
// Selects, at compile time, the implementation wrapped by Os::Cpu: include the header defining the implementation and
// name it Os::Selected::Cpu. Os/Cpu.hpp defines Os::Cpu as Os::CpuCommon<Os::Selected::Cpu>, which holds the
// implementation by value and forwards to it without virtual dispatch.
//
// The implementation header must not include Os/Cpu.hpp (it derives from Os/Interfaces/CpuInterface.hpp). The alias
// only changes the C++ type; the module providing the implementation must still be in the link.
// ======================================================================
#ifndef CONFIG_CPUSELECTION_HPP
#define CONFIG_CPUSELECTION_HPP

#if defined(TGT_OS_TYPE_LINUX)
#include <Os/Linux/Cpu.hpp>
#elif defined(TGT_OS_TYPE_DARWIN)
#include <Os/Darwin/Cpu.hpp>
#else
#error "No default Os::Cpu implementation for this platform: override config/CpuSelection.hpp"
#endif

namespace Os {
namespace Selected {
#if defined(TGT_OS_TYPE_LINUX)
using Cpu = Linux::Cpu::LinuxCpu;
#elif defined(TGT_OS_TYPE_DARWIN)
using Cpu = Darwin::Cpu::DarwinCpu;
#endif
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_CPUSELECTION_HPP
