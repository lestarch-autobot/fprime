// ======================================================================
// \title Os/Interfaces/ConsoleUserInterface.hpp
// \brief types shared by Os::Console users and implementations
//
// Includes no other Os service header, so both Os/Console.hpp and the Os::Console implementations may include it.
// ======================================================================
#ifndef OS_INTERFACES_CONSOLEUSERINTERFACE_HPP_
#define OS_INTERFACES_CONSOLEUSERINTERFACE_HPP_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Logger/Logger.hpp>
#include <Fw/Types/Assert.hpp>
#include <Os/Os.hpp>

namespace Os {

//! \brief Base class for storing implementation specific handle information
struct ConsoleHandle {};

//! \brief types and constants shared by users of Os::Console and by its implementations
struct ConsoleUserInterface {};

}  // namespace Os

#endif  // OS_INTERFACES_CONSOLEUSERINTERFACE_HPP_
