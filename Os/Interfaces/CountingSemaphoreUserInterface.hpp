// ======================================================================
// \title Os/Interfaces/CountingSemaphoreUserInterface.hpp
// \brief types shared by Os::CountingSemaphore users and implementations
//
// Includes no other Os service header, so both Os/CountingSemaphore.hpp and the Os::CountingSemaphore implementations
// may include it.
// ======================================================================
#ifndef OS_INTERFACES_COUNTINGSEMAPHOREUSERINTERFACE_HPP_
#define OS_INTERFACES_COUNTINGSEMAPHOREUSERINTERFACE_HPP_

#include <Fw/Time/TimeInterval.hpp>
#include <Fw/Types/Assert.hpp>
#include <Os/Os.hpp>

namespace Os {

class CountingSemaphoreHandle {};

//! \brief types and constants shared by users of Os::CountingSemaphore and by its implementations
struct CountingSemaphoreUserInterface {
    enum Status {
        OP_OK,                  //!< Operation was successful
        ERROR_TIMEOUT,          //!< Timeout occurred during wait
        ERROR_INVALID,          //!< Invalid semaphore or argument
        ERROR_NOT_IMPLEMENTED,  //!< Feature not implemented
        NOT_SUPPORTED,          //!< CountingSemaphore does not support operation
        ERROR_OTHER             //!< All other errors
    };
};

}  // namespace Os

#endif  // OS_INTERFACES_COUNTINGSEMAPHOREUSERINTERFACE_HPP_
