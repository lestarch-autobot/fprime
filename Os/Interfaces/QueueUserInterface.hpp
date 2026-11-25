// ======================================================================
// \title Os/Interfaces/QueueUserInterface.hpp
// \brief types shared by Os::Queue users and implementations
//
// Includes no other Os service header, so both Os/Queue.hpp and the Os::Queue implementations may include it.
// ======================================================================
#ifndef OS_INTERFACES_QUEUEUSERINTERFACE_HPP_
#define OS_INTERFACES_QUEUEUSERINTERFACE_HPP_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Obj/ObjBase.hpp>
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Os/Os.hpp>
#include <Os/QueueString.hpp>

namespace Os {

//! \brief QueueHandle parent class
class QueueHandle {};

//! \brief types and constants shared by users of Os::Queue and by its implementations
struct QueueUserInterface {
    //! \brief status returned from the queue send function
    enum Status {
        OP_OK,              //!<  message sent/received okay
        ALREADY_CREATED,    //!<  creating an already created queue
        EMPTY,              //!<  If non-blocking, all the messages have been drained.
        UNINITIALIZED,      //!<  Queue wasn't initialized successfully
        SIZE_MISMATCH,      //!<  attempted to send or receive with buffer too large, too small
        SEND_ERROR,         //!<  message send error
        RECEIVE_ERROR,      //!<  message receive error
        INVALID_PRIORITY,   //!<  invalid priority requested
        FULL,               //!<  Queue was full when attempting to send a message
        NOT_SUPPORTED,      //!<  Queue feature is not supported
        ALLOCATION_FAILED,  //!<  required memory could not be allocated
        UNKNOWN_ERROR       //!<  Unexpected error; can't match with returns
    };

    //! \brief message type
    enum BlockingType {
        BLOCKING,    //!< Message will block until space is available
        NONBLOCKING  //!< Message will return with status when space is unavailable
    };
};

}  // namespace Os

#endif  // OS_INTERFACES_QUEUEUSERINTERFACE_HPP_
