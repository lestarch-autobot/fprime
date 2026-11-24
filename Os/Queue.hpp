// ======================================================================
// \title Os/Queue.hpp
// \brief common function definitions for Os::Queue
// ======================================================================
#ifndef Os_Queue_hpp_
#define Os_Queue_hpp_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Obj/ObjBase.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Os/Mutex.hpp>
#include <Os/Os.hpp>
#include <Os/QueueString.hpp>
#include "config/OsSelection.hpp"  // defines the Os::Queue alias and OS_QUEUE_HEADER; do not remove
namespace Os {
// Forward declaration for registry
class QueueRegistry;

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

template <typename T>
class QueueCommon final : public QueueUserInterface {
  public:
    //! \brief queue constructor
    QueueCommon();

    //! \brief default queue destructor
    virtual ~QueueCommon();

    //! \brief copy constructor is forbidden
    QueueCommon(const QueueCommon& other) = delete;

    //! \brief copy constructor is forbidden
    QueueCommon(const QueueCommon* other) = delete;

    //! \brief assignment operator is forbidden
    QueueCommon& operator=(const QueueCommon& other) = delete;

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
    Status create(FwEnumStoreType id, const Fw::ConstStringBase& name, FwSizeType depth, FwSizeType messageSize);

    //! \brief teardown the queue
    //!
    //! Allow for queues to deallocate resources as part of system shutdown. This delegates to the underlying queue
    //! implementation.
    //! implementation.
    void teardown();

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
    Status send(const U8* buffer, FwSizeType size, FwQueuePriorityType priority, BlockingType blockType);

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
                   FwQueuePriorityType& priority);

    //! \brief get number of messages available
    //!
    //! Returns the number of messages currently available in the queue. This method delegates to the underlying
    //! implementation.
    //!
    //! \return number of messages available
    FwSizeType getMessagesAvailable() const;

    //! \brief get maximum messages stored at any given time through delegate
    //!
    //! Returns the maximum number of messages in this queue at any given time. This is the high-water mark for
    //! this queue.
    //! \return queue message high-water mark
    FwSizeType getMessageHighWaterMark() const;

    //! \brief return the underlying queue handle (implementation specific). Delegates to implementation.
    //! \return internal task handle representation
    QueueHandle* getHandle();

    //! \brief send a message to a queue
    //!
    //! Send a message to a queue with the given priority and block type. See: QueueUserInterface::send
    //!
    //! \warning This method will block if the queue is full and blockType is set to BLOCKING
    //!
    //! \param message: reference to serialize buffer storing message
    //! \param priority: priority of the message
    //! \param blockType: BLOCKING to block for space or NONBLOCKING to return error when queue is full
    //! \return status of the send
    Status send(const Fw::LinearBufferBase& message, FwQueuePriorityType priority, BlockingType blockType);

    //! \brief receive a message from a queue
    //!
    //! Receive a message from a queue with the given block type. See: QueueUserInterface::receive. Note: this will
    //! entirely overwrite the buffer.
    //!
    //! \warning This method will block if the queue is full and blockType is set to BLOCKING
    //!
    //! \param destination: reference to serialize buffer for storing message
    //! \param priority: (output) priority of the message
    //! \param blockType: BLOCKING to block for space or NONBLOCKING to return error when queue is full
    //! \return status of the send
    Status receive(Fw::LinearBufferBase& destination, BlockingType blockType, FwQueuePriorityType& priority);

    //! \brief get the queue's depth in messages
    FwSizeType getDepth() const;

    //! \brief get the queue's message maximum size
    FwSizeType getMessageSize() const;

    //! \brief get the queue's name
    const QueueString& getName() const;

    //! \brief get number of queues system-wide
    static FwSizeType getNumQueues();

    //! \brief get static mutex
    static Os::Mutex& getStaticMutex();

  private:
    QueueString m_name;              //!< queue name
    FwSizeType m_depth;              //!< Queue depth
    FwSizeType m_size;               //!< Maximum message size
    static Os::Mutex s_countLock;    //!< Lock the count
    static FwSizeType s_queueCount;  //!< Count of the number of queues

#if FW_QUEUE_REGISTRATION
  public:
    //! \brief set QueueRegistry for tracking queues
    //!
    //! \param registry: registry to set
    static void setRegistry(QueueRegistry* registry);

  private:
    static QueueRegistry* s_queueRegistry;  //!< Queue registry store
#endif

    // This section is used to store the implementation-defined file handle. To Os::File and fprime, this type is
    // opaque and thus normal allocation cannot be done. Instead, we allow the implementor to store then handle in
    // the byte-array here and set `handle` to that address for storage.
    //
    T m_delegate;  //!< Delegate for the real implementation
};

//! \brief queue registry interface
//!
//! The QueueRegistry is used to track queues in the system. There is intended to be a single, global, queue registry
//! across the system. It is used to track queues and will receive a callback on the creation of each queue.
class QueueRegistry {
  public:
    //! Default QueueRegistry
    QueueRegistry() = default;
    //! Default ~QueueRegistry
    virtual ~QueueRegistry() = default;

    //! \brief queue registry callback
    //!
    //! Register the queue with this queue registry. Must be implemented by QueueRegistry implementations.
    //!
    //! \param queue: queue being registered
    virtual void registerQueue(Queue* queue) = 0;  //!< method called by queue init() methods to register a new queue
};
}  // namespace Os
// Validate that OS_QUEUE_HEADER was defined by config/OsSelection.hpp
#ifndef OS_QUEUE_HEADER
#error "OS_QUEUE_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_QUEUE_HEADER

#endif
