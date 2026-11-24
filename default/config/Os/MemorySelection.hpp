// ======================================================================
// \title config/MemorySelection.hpp
// \brief configured selection of the Os::Memory implementation
//
// Selects, at compile time, the implementation wrapped by Os::Memory: include the header defining the implementation
// and name it Os::Selected::Memory. Os/Memory.hpp defines Os::Memory as Os::MemoryCommon<Os::Selected::Memory>, which
// holds the implementation by value and forwards to it without virtual dispatch.
//
// The implementation header must not include Os/Memory.hpp (it derives from Os/Interfaces/MemoryInterface.hpp). The
// alias only changes the C++ type; the module providing the implementation must still be in the link.
// ======================================================================
#ifndef CONFIG_MEMORYSELECTION_HPP
#define CONFIG_MEMORYSELECTION_HPP

#if defined(TGT_OS_TYPE_LINUX)
#include <Os/Linux/Memory.hpp>
#elif defined(TGT_OS_TYPE_DARWIN)
#include <Os/Darwin/Memory.hpp>
#else
#error "No default Os::Memory implementation for this platform: override config/MemorySelection.hpp"
#endif

namespace Os {
namespace Selected {
#if defined(TGT_OS_TYPE_LINUX)
using Memory = Linux::Memory::LinuxMemory;
#elif defined(TGT_OS_TYPE_DARWIN)
using Memory = Darwin::Memory::DarwinMemory;
#endif
}  // namespace Selected
}  // namespace Os

#endif  // CONFIG_MEMORYSELECTION_HPP
