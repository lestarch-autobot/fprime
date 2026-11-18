// ======================================================================
// \title Os/Cpu.hpp
// \brief public Os::Cpu interface and alias
//
// This header aggregates all definitions needed to use Os::Cpu:
// the interface, the configured alias, and the concrete selected type.
//
// WARNING — include order is load-bearing. Do not reorder.
//
// The dependency constraints are:
//
//   1. config/OsSelection.hpp (CFG) defines the Os::Cpu type alias by
//      forward-declaring a link-time delegate (DelegateCpu) or directly
//      aliasing a concrete implementation (e.g. Os::Linux::Cpu::LinuxCpu).
//      Must not include Os OSAL headers (they aren't yet defined).
//      Should only be included in Os/*Interface.hpp.
//
//   2. Os/CpuInterface.hpp (IF) needs the Os::Cpu alias in its function
//      signatures, so it includes CFG first. It then defines CpuHandle
//      and CpuInterface.
//
//   3. OS_CPU_HEADER (IMPL) is defined by CFG and points to the concrete
//      implementation header. If using delegation, this points to
//      Os/DelegateCpu.hpp. If using compile-time selection, it points
//      directly to a platform-specific implementation (e.g. Os/Linux/Cpu.hpp).
//
// CpuInterface.hpp must precede OS_CPU_HEADER here,
// and CFG must never include either of them (that would form a cycle).
// ======================================================================
#ifndef OS_CPU_HPP_
#define OS_CPU_HPP_

#include "Os/CpuInterface.hpp"

#include OS_CPU_HEADER

#endif  // OS_CPU_HPP_
