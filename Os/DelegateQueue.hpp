// ======================================================================
// \title Os/DelegateQueue.hpp
// \brief Define the Os::DelegateQueue class
// ======================================================================
#ifndef OS_DELEGATEQUEUE_HPP_
#define OS_DELEGATEQUEUE_HPP_

#include "Os/QueueInterface.hpp"

namespace Os {

//! \brief Link-time delegating Queue implementation.
//!
//! Stores an implementation-defined queue in a byte array and forwards all operations to a delegate constructed
//! (via placement-new) by QueueInterface::getDelegate(). Which getDelegate() is linked selects the concrete
//! implementation at link time. This is the default binding of the Os::Queue alias; platforms may instead alias
//! Os::Queue directly to a concrete implementation for compile-time selection (see config/OsSelection.hpp).
class DelegateQueue final : public QueueInterface {
  public:
    //! \brief queue constructor
    DelegateQueue();

    //! \brief default queue destructor
    virtual ~DelegateQueue();

    //! \brief copy constructor is forbidden
    DelegateQueue(const DelegateQueue& other) = delete;

    //! \brief copy constructor is forbidden
    DelegateQueue(const DelegateQueue* other) = delete;

    //! \brief assignment operator is forbidden
    QueueInterface& operator=(const QueueInterface& other) override = delete;

    //! \brief create queue storage through delegate
    //!
    //! Creates a queue ensuring sufficient storage to hold `depth` messages of `messageSize` size each. This method
    //! delegates to the underlying implementation. Resource allocation is dependent on the underlying implementation
    //! and users should assume that resource allocation is possible.
    //!
    //! \param name: name of queue
    //! \param depth: depth of queue in number of messages
    //! \param messageSize: size of an individual message
    //! \return: status of the creation
    Status create(FwEnumStoreType id,
                  const Fw::ConstStringBase& name,
                  FwSizeType depth,
                  FwSizeType messageSize) override;

    //! \brief teardown the queue
    //!
    //! Allow for queues to deallocate resources as part of system shutdown. This delegates to the underlying queue
    //! implementation. The queue is uninitialized afterward and may be created again.
    void teardown() override;

    // Bring the common send/receive overloads of the interface back into scope
    using QueueInterface::receive;
    using QueueInterface::send;

    //! \brief send a message into the queue through delegate
    //!
    //! Send a message into the queue, providing the message data, size, priority, and blocking type. When
    //! `blockType` is set to BLOCKING, this call will block on queue full. Otherwise, this will return an error
    //! status on queue full. This method delegates to the underlying implementation.
    //!
    //! \warning It is invalid to send a null buffer
    //! \warning This method will block if the queue is full and blockType is set to BLOCKING
    //!
    //! \param buffer: message data
    //! \param size: size of message data
    //! \param priority: priority of the message
    //! \param blockType: BLOCKING to block for space or NONBLOCKING to return error when queue is full
    //! \return: status of the send
    Status send(const U8* buffer, FwSizeType size, FwQueuePriorityType priority, BlockingType blockType) override;

    //! \brief receive a message from the queue through delegate
    //!
    //! Receive a message from the queue, providing the message destination, capacity, priority, and blocking type.
    //! When `blockType` is set to BLOCKING, this call will block on queue empty. Otherwise, this will return an
    //! error status on queue empty. Actual size received and priority of message is set on success status. This method
    //! delegates to the underlying implementation.
    //!
    //! \warning It is invalid to send a null buffer.
    //! \warning This method will block if the queue is empty and blockType is set to BLOCKING
    //!
    //! \param destination: destination for message data
    //! \param capacity: maximum size of message data
    //! \param blockType: BLOCKING to wait for message or NONBLOCKING to return error when queue is empty
    //! \param actualSize: (output) actual size of message read
    //! \param priority: (output) priority of message read
    //! \return: status of the send
    Status receive(U8* destination,
                   FwSizeType capacity,
                   BlockingType blockType,
                   FwSizeType& actualSize,
                   FwQueuePriorityType& priority) override;

    //! \brief get number of messages available
    //!
    //! Returns the number of messages currently available in the queue. This method delegates to the underlying
    //! implementation.
    //!
    //! \return number of messages available
    FwSizeType getMessagesAvailable() const override;

    //! \brief get maximum messages stored at any given time through delegate
    //!
    //! Returns the maximum number of messages in this queue at any given time. This is the high-water mark for
    //! this queue.
    //! \return queue message high-water mark
    FwSizeType getMessageHighWaterMark() const override;

    //! \brief return the underlying queue handle (implementation specific). Delegates to implementation.
    //! \return internal task handle representation
    QueueHandle* getHandle() override;

  private:
    // This section is used to store the implementation-defined queue handle. To Os::Queue and fprime, this type is
    // opaque and thus normal allocation cannot be done. Instead, we allow the implementor to store then handle in
    // the byte-array here and set `handle` to that address for storage.
    //
    alignas(FW_HANDLE_ALIGNMENT) QueueHandleStorage m_handle_storage;  //!< Storage for aligned handle
    QueueInterface& m_delegate;                                        //!< Delegate for the real implementation
};

}  // namespace Os

#endif  // OS_DELEGATEQUEUE_HPP_
