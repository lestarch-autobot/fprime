// ======================================================================
// \title Os/Memory.hpp
// \brief public Os::Memory interface and alias
//
// Include order is load-bearing: Os/MemoryInterface.hpp (which includes
// config/OsSelection.hpp) must precede OS_MEMORY_HEADER. See Os/RawTime.hpp.
// ======================================================================
#ifndef OS_MEMORY_HPP_
#define OS_MEMORY_HPP_

#include "Os/MemoryInterface.hpp"

#include OS_MEMORY_HEADER

#include <type_traits>

static_assert(sizeof(Os::Memory) > 0, "OS_MEMORY_HEADER must define the class aliased as Os::Memory");
static_assert(std::is_base_of<Os::MemoryInterface, Os::Memory>::value,
              "Os::Memory (config/OsSelection.hpp) must derive from Os::MemoryInterface");

#endif  // OS_MEMORY_HPP_
