// ======================================================================
// \title Os/Interfaces/QueueInterface.hpp
// \brief pure-virtual interface implemented by Os::Queue implementations
// ======================================================================
// Included before the guard: Os/Queue.hpp includes the selected implementation, which includes this file
#include <Os/Queue.hpp>

#ifndef OS_INTERFACES_QUEUEINTERFACE_HPP_
#define OS_INTERFACES_QUEUEINTERFACE_HPP_

namespace Os {

//! \brief base queue interface
//!
//! Queues are used internally to fprime in order to support the messaging between components. The
//! QueueInterface is used to abstract away from the standard OS-based queue, allowing F prime support
//! multiple OSes in a consistent way.
//!
class QueueInterface : public QueueUserInterface {
  public:
    //! \brief default queue interface constructor
    QueueInterface() = default;

    //! \brief default queue destructor
    virtual ~QueueInterface() = default;

    //! \brief copy constructor is forbidden
    QueueInterface(const QueueInterface& other) = delete;

    //! \brief copy constructor is forbidden
    QueueInterface(const QueueInterface* other) = delete;

    //! \brief assignment operator is forbidden
    virtual QueueInterface& operator=(const QueueInterface& other) = delete;  // NO_CODESONAR (cpp:S3657)

    //! \brief create queue storage
    //!
    //! Creates a queue ensuring sufficient storage to hold `depth` messages of `messageSize` size each. Resource
    //! allocation is dependent on the underlying implementation and users should assume that resource allocation is
    //! possible.
    //!
    //! \param id: identifier for the queue, used for memory allocation
    //! \param name: name of queue
    //! \param depth: depth of queue in number of messages
    //! \param messageSize: size of an individual message
    //! \return: status of the creation
    virtual Status create(FwEnumStoreType id,
                          const Fw::ConstStringBase& name,
                          FwSizeType depth,
                          FwSizeType messageSize) = 0;

    //! \brief teardown the queue
    //!
    //! Allow for queues to deallocate resources as part of system shutdown. This delegates to the underlying queue
    //! implementation.
    virtual void teardown() = 0;

    //! \brief send a message into the queue
    //!
    //! Send a message into the queue, providing the message data, size, priority, and blocking type. When
    //! `blockType` is set to BLOCKING, this call will block on queue full. Otherwise, this will return an error
    //! status on queue full.
    //!
    //! It is invalid to send a null buffer.
    //! This method will block if the queue is full and blockType is set to BLOCKING
    //!
    //! \param buffer: message data
    //! \param size: size of message data
    //! \param priority: priority of the message
    //! \param blockType: BLOCKING to block for space or NONBLOCKING to return error when queue is full
    //! \return: status of the send
    virtual Status send(const U8* buffer, FwSizeType size, FwQueuePriorityType priority, BlockingType blockType) = 0;

    //! \brief receive a message from the queue
    //!
    //! Receive a message from the queue, providing the message destination, capacity, priority, and blocking type.
    //! When `blockType` is set to BLOCKING, this call will block on queue empty. Otherwise, this will return an
    //! error status on queue empty. Actual size received and priority of message is set on success status.
    //!
    //! It is invalid to send a null buffer.
    //! This method will block if the queue is empty and blockType is set to BLOCKING
    //!
    //! \param destination: destination for message data
    //! \param capacity: maximum size of message data
    //! \param blockType: BLOCKING to wait for message or NONBLOCKING to return error when queue is empty
    //! \param actualSize: (output) actual size of message read
    //! \param priority: (output) priority of message read
    //! \return: status of the send
    virtual Status receive(U8* destination,
                           FwSizeType capacity,
                           BlockingType blockType,
                           FwSizeType& actualSize,
                           FwQueuePriorityType& priority) = 0;

    //! \brief get number of messages available
    //!
    //! Returns the number of messages currently available in the queue.
    //!
    //! \return number of messages available
    virtual FwSizeType getMessagesAvailable() const = 0;

    //! \brief get maximum messages stored at any given time
    //!
    //! Returns the maximum number of messages in this queue at any given time. This is the high-water mark for this
    //! queue.
    //! \return queue message high-water mark
    virtual FwSizeType getMessageHighWaterMark() const = 0;

    //! \brief return the underlying queue handle (implementation specific)
    //! \return internal task handle representation
    virtual QueueHandle* getHandle() = 0;
};

}  // namespace Os

#endif  // OS_INTERFACES_QUEUEINTERFACE_HPP_
