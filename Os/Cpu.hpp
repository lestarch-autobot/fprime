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
// The common CpuInterface functions are defined inline below, once Os::Cpu
// is complete.
//
// CpuInterface.hpp must precede OS_CPU_HEADER here,
// and CFG must never include either of them (that would form a cycle).
// ======================================================================
#ifndef OS_CPU_HPP_
#define OS_CPU_HPP_

#include "Os/CpuInterface.hpp"

#include OS_CPU_HEADER

#include <type_traits>

// ------------------------------------------------------------
// CpuInterface common implementations
//
// Built on the configured Os::Cpu and the pure virtual functions. Defined
// here, where Os::Cpu is complete, so that the code using Os::Cpu is what
// references the selected class: a compile-time selection then needs neither
// Os::DelegateCpu nor CpuInterface::getDelegate().
// ------------------------------------------------------------
namespace Os {

static_assert(std::is_convertible<Os::Cpu*, CpuInterface*>::value,
              "Os::Cpu must publicly derive from Os::CpuInterface");
static_assert(std::is_default_constructible<Os::Cpu>::value, "Os::Cpu must be default constructible");

inline Os::Cpu& CpuInterface::getSingleton() {
    static Os::Cpu _singleton;
    return _singleton;
}

inline void CpuInterface::init() {
    (void)CpuInterface::getSingleton();
}

inline CpuInterface::Status CpuInterface::getCount(FwSizeType& cpu_count) {
    return CpuInterface::getSingleton()._getCount(cpu_count);
}

inline CpuInterface::Status CpuInterface::getTicks(Ticks& ticks, FwSizeType cpu_index) {
    return CpuInterface::getSingleton()._getTicks(ticks, cpu_index);
}

}  // namespace Os

#endif  // OS_CPU_HPP_
