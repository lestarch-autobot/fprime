// ======================================================================
// \title Os/Queue.hpp
// \brief common function definitions for Os::Queue
// ======================================================================
#ifndef Os_Queue_hpp_
#define Os_Queue_hpp_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Obj/ObjBase.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Os/Interfaces/QueueUserInterface.hpp>
#include <Os/Mutex.hpp>
#include <Os/Os.hpp>
#include <Os/QueueString.hpp>
#include "Fw/Types/Assert.hpp"
#include "config/QueueSelection.hpp"
#ifdef BUILD_UT
#include <Os/test/ut/implementations/UtQueue.hpp>
#endif
namespace Os {

template <typename T>
class QueueCommon;

#ifdef BUILD_UT
//! \brief Os::Queue in unit test builds: wraps the implementation selected in config/QueueSelection.hpp in the
//! test-only UtQueue, so that tests may intercept its calls
using Queue = QueueCommon<Ut::Queue::UtQueue>;
#else
//! \brief Os::Queue: the common class on the implementation selected in config/QueueSelection.hpp
using Queue = QueueCommon<Selected::Queue>;
#endif
// Forward declaration for registry
class QueueRegistry;

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

    //! \brief register a queue of the type selected in config/QueueSelection.hpp (Os::Queue)
    static void registerQueue(QueueRegistry& registry, Queue* queue);
    //! \brief queues of any other type are not tracked: the registry handles Os::Queue only
    static void registerQueue(QueueRegistry& registry, const void* queue);
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
namespace Os {

template <typename T>
FwSizeType QueueCommon<T>::s_queueCount = 0;
#if FW_QUEUE_REGISTRATION
template <typename T>
QueueRegistry* QueueCommon<T>::s_queueRegistry = nullptr;
#endif

template <typename T>
QueueCommon<T>::QueueCommon() : m_name(""), m_depth(0), m_size(0), m_delegate() {}

template <typename T>
QueueCommon<T>::~QueueCommon() {}

template <typename T>
QueueUserInterface::Status QueueCommon<T>::create(FwEnumStoreType id,
                                                  const Fw::ConstStringBase& name,
                                                  FwSizeType depth,
                                                  FwSizeType messageSize) {
    FW_ASSERT(depth > 0);
    FW_ASSERT(messageSize > 0);
    // Check for previous creation call
    if (this->m_depth > 0 || this->m_size > 0) {
        return QueueUserInterface::Status::ALREADY_CREATED;
    }
    QueueUserInterface::Status status = this->m_delegate.create(id, name, depth, messageSize);
    if (status == QueueUserInterface::Status::OP_OK) {
        this->m_name = name;
        this->m_depth = depth;
        this->m_size = messageSize;
        ScopeLock lock(QueueCommon::getStaticMutex());
        QueueCommon::s_queueCount++;
#if FW_QUEUE_REGISTRATION
        if (QueueCommon::s_queueRegistry != nullptr) {
            QueueCommon::registerQueue(*QueueCommon::s_queueRegistry, this);
        }
#endif
    }
    return status;
}

template <typename T>
void QueueCommon<T>::teardown() {
    return this->m_delegate.teardown();
}

template <typename T>
QueueUserInterface::Status QueueCommon<T>::send(const U8* buffer,
                                                FwSizeType size,
                                                FwQueuePriorityType priority,
                                                QueueUserInterface::BlockingType blockType) {
    FW_ASSERT(buffer != nullptr);
    // Check if initialized
    if (this->m_depth == 0 || this->m_size == 0) {
        return QueueUserInterface::Status::UNINITIALIZED;
    }
    // Check size before proceeding
    else if (size > this->getMessageSize()) {
        return QueueUserInterface::Status::SIZE_MISMATCH;
    }
    return this->m_delegate.send(buffer, size, priority, blockType);
}

template <typename T>
QueueUserInterface::Status QueueCommon<T>::receive(U8* destination,
                                                   FwSizeType capacity,
                                                   QueueUserInterface::BlockingType blockType,
                                                   FwSizeType& actualSize,
                                                   FwQueuePriorityType& priority) {
    FW_ASSERT(destination != nullptr);
    // Check if initialized
    if (this->m_depth == 0 || this->m_size == 0) {
        return QueueUserInterface::Status::UNINITIALIZED;
    }
    // Check capacity before proceeding
    else if (capacity < this->getMessageSize()) {
        return QueueUserInterface::Status::SIZE_MISMATCH;
    }
    return this->m_delegate.receive(destination, capacity, blockType, actualSize, priority);
}

template <typename T>
FwSizeType QueueCommon<T>::getMessagesAvailable() const {
    return this->m_delegate.getMessagesAvailable();
}

template <typename T>
FwSizeType QueueCommon<T>::getMessageHighWaterMark() const {
    return this->m_delegate.getMessageHighWaterMark();
}

template <typename T>
QueueHandle* QueueCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

template <typename T>
QueueUserInterface::Status QueueCommon<T>::send(const Fw::LinearBufferBase& message,
                                                FwQueuePriorityType priority,
                                                QueueUserInterface::BlockingType blockType) {
    return this->send(message.getBuffAddr(), message.getSize(), priority, blockType);
}

template <typename T>
QueueUserInterface::Status QueueCommon<T>::receive(Fw::LinearBufferBase& destination,
                                                   QueueUserInterface::BlockingType blockType,
                                                   FwQueuePriorityType& priority) {
    FwSizeType actualSize = 0;
    destination.resetSer();  // Reset the buffer
    QueueUserInterface::Status status =
        this->receive(destination.getBuffAddrSer(), destination.getCapacity(), blockType, actualSize, priority);
    if (status == QueueUserInterface::Status::OP_OK) {
        Fw::SerializeStatus serializeStatus =
            destination.setBuffLen(static_cast<Fw::Serializable::SizeType>(actualSize));
        if (serializeStatus != Fw::SerializeStatus::FW_SERIALIZE_OK) {
            status = QueueUserInterface::Status::SIZE_MISMATCH;
        }
    }
    return status;
}

template <typename T>
FwSizeType QueueCommon<T>::getDepth() const {
    return this->m_depth;
}

template <typename T>
FwSizeType QueueCommon<T>::getMessageSize() const {
    return this->m_size;
}

template <typename T>
const QueueString& QueueCommon<T>::getName() const {
    return this->m_name;
}

template <typename T>
FwSizeType QueueCommon<T>::getNumQueues() {
    ScopeLock lock(QueueCommon::getStaticMutex());
    return QueueCommon::s_queueCount;
}

template <typename T>
Os::Mutex& QueueCommon<T>::getStaticMutex() {
    static Os::Mutex s_mutex;
    return s_mutex;
}

#if FW_QUEUE_REGISTRATION
template <typename T>
void QueueCommon<T>::registerQueue(QueueRegistry& registry, Queue* queue) {
    registry.registerQueue(queue);
}

template <typename T>
void QueueCommon<T>::registerQueue(QueueRegistry& registry, const void* queue) {}

template <typename T>
void QueueCommon<T>::setRegistry(QueueRegistry* registry) {
    ScopeLock lock(QueueCommon::getStaticMutex());
    QueueCommon::s_queueRegistry = registry;
}
#endif

}  // namespace Os

#endif
