// ======================================================================
// \title Os/Interfaces/MemoryUserInterface.hpp
// \brief types shared by Os::Memory users and implementations
//
// Includes no other Os service header, so both Os/Memory.hpp and the Os::Memory implementations may include it.
// ======================================================================
#ifndef OS_INTERFACES_MEMORYUSERINTERFACE_HPP_
#define OS_INTERFACES_MEMORYUSERINTERFACE_HPP_

#include <Fw/Types/Assert.hpp>
#include <Os/Os.hpp>

namespace Os {

//! \brief Memory variable handle parent
class MemoryHandle {};

//! \brief types and constants shared by users of Os::Memory and by its implementations
struct MemoryUserInterface {
    using Status = Os::Generic::Status;
    using Usage = Os::Generic::UsedTotal;
};

}  // namespace Os

#endif  // OS_INTERFACES_MEMORYUSERINTERFACE_HPP_
