// ======================================================================
// \title config/MemorySelection.hpp
// \brief selection of Linux::Memory::LinuxMemory as the Os::Memory implementation
//
// Supplied as one of the CONFIGURATION_HEADERS of the implementation module: the build system places this file at
// config/MemorySelection.hpp when that implementation is chosen (CHOOSES_IMPLEMENTATIONS in register_fprime_config).
// Os/Memory.hpp then defines Os::Memory as Os::MemoryCommon<Os::Selected::Memory>.
// ======================================================================
#ifndef CONFIG_MEMORYSELECTION_HPP
#define CONFIG_MEMORYSELECTION_HPP

#include <Os/Linux/Memory.hpp>

namespace Os {
namespace Selected {
using Memory = Linux::Memory::LinuxMemory;
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_MEMORYSELECTION_HPP
