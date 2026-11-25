// ======================================================================
// \title Os/Interfaces/RawTimeUserInterface.hpp
// \brief types shared by Os::RawTime users and implementations
//
// Includes no other Os service header, so both Os/RawTime.hpp and the Os::RawTime implementations may include it.
// ======================================================================
#ifndef OS_INTERFACES_RAWTIMEUSERINTERFACE_HPP_
#define OS_INTERFACES_RAWTIMEUSERINTERFACE_HPP_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Time/TimeInterval.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Os/Os.hpp>
#include <limits>
#include "config/RawTimeSource.hpp"

namespace Os {

struct RawTimeHandle {};

//! \brief types shared by Os::RawTime users and implementations
struct RawTimeUserInterface {
    // Serialization size for RawTime objects, configured in config/FpConfig.h
    static const FwSizeType SERIALIZED_SIZE = FW_RAW_TIME_SERIALIZATION_MAX_SIZE;

    enum Status {
        OP_OK,           //!< Operation was successful
        OP_OVERFLOW,     //!< Operation result caused an overflow
        INVALID_PARAMS,  //!< Parameters invalid for current platform
        NOT_SUPPORTED,   //!< RawTime does not support operation
        OTHER_ERROR      //!< All other errors
    };
};

}  // namespace Os

#endif  // OS_INTERFACES_RAWTIMEUSERINTERFACE_HPP_
