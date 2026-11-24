// ======================================================================
// \title Os/Interfaces/CpuUserInterface.hpp
// \brief types shared by Os::Cpu users and implementations
//
// Includes no other Os service header, so both Os/Cpu.hpp and the Os::Cpu implementations may include it.
// ======================================================================
#ifndef OS_INTERFACES_CPUUSERINTERFACE_HPP_
#define OS_INTERFACES_CPUUSERINTERFACE_HPP_

#include <Fw/Types/Assert.hpp>
#include <Os/Os.hpp>

namespace Os {

//! \brief Cpu variable handle parent
class CpuHandle {};

//! \brief types and constants shared by users of Os::Cpu and by its implementations
struct CpuUserInterface {
    using Status = Os::Generic::Status;
    using Ticks = Os::Generic::UsedTotal;
};

}  // namespace Os

#endif  // OS_INTERFACES_CPUUSERINTERFACE_HPP_
