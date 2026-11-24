// ======================================================================
// \title Os/Interfaces/MutexUserInterface.hpp
// \brief types shared by Os::Mutex users and implementations
//
// Includes no other Os service header, so both Os/Mutex.hpp and the Os::Mutex implementations may include it.
// ======================================================================
#ifndef OS_INTERFACES_MUTEXUSERINTERFACE_HPP_
#define OS_INTERFACES_MUTEXUSERINTERFACE_HPP_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Types/Assert.hpp>
#include <Os/Os.hpp>

namespace Os {

struct MutexHandle {};

//! \brief types shared by Os::Mutex users and implementations
struct MutexUserInterface {
    enum Status {
        OP_OK,           //!< Operation was successful
        ERROR_BUSY,      //!< Mutex is busy
        ERROR_DEADLOCK,  //!< Deadlock condition detected
        NOT_SUPPORTED,   //!< Mutex does not support operation
        ERROR_OTHER      //!< All other errors
    };
};

}  // namespace Os

#endif  // OS_INTERFACES_MUTEXUSERINTERFACE_HPP_
