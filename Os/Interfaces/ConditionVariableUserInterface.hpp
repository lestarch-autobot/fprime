// ======================================================================
// \title Os/Interfaces/ConditionVariableUserInterface.hpp
// \brief types shared by Os::ConditionVariable users and implementations
//
// Includes no other Os service header, so both Os/Condition.hpp and the Os::ConditionVariable implementations may
// include it.
// ======================================================================
#ifndef OS_INTERFACES_CONDITIONVARIABLEUSERINTERFACE_HPP_
#define OS_INTERFACES_CONDITIONVARIABLEUSERINTERFACE_HPP_

#include <Fw/Types/Assert.hpp>
#include <Os/Os.hpp>

namespace Os {

//! \brief Condition variable handle parent
class ConditionVariableHandle {};

//! \brief types shared by Os::ConditionVariable users and implementations
struct ConditionVariableUserInterface {
    enum Status {
        OP_OK,                  //!<  Operation was successful
        ERROR_MUTEX_NOT_HELD,   //!< When trying to wait but we don't hold the mutex
        ERROR_DIFFERENT_MUTEX,  //!< When trying to use a different mutex than expected mutex
        ERROR_NOT_IMPLEMENTED,  //!< When trying to use a feature that isn't implemented
        NOT_SUPPORTED,          //!< ConditionVariable does not support operation
        ERROR_OTHER             //!< All other errors
    };
};

}  // namespace Os

#endif  // OS_INTERFACES_CONDITIONVARIABLEUSERINTERFACE_HPP_
