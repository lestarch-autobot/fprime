// ======================================================================
// \title config/CpuSelection.hpp
// \brief selection of Darwin::Cpu::DarwinCpu as the Os::Cpu implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/CpuSelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in register_fprime_config).
// Os/Cpu.hpp then defines Os::Cpu as Os::CpuCommon<Os::Selected::Cpu>.
// ======================================================================
#ifndef CONFIG_CPUSELECTION_HPP
#define CONFIG_CPUSELECTION_HPP

#include <Os/Darwin/Cpu.hpp>

namespace Os {
namespace Selected {
using Cpu = Darwin::Cpu::DarwinCpu;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_CPUSELECTION_HPP
