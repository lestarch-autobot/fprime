// ======================================================================
// \title Os/Queue.cpp
// \brief common function implementation for Os::Queue
// ======================================================================
#include "Os/Queue.hpp"
#include "Fw/Types/Assert.hpp"
#include "Fw/Types/Serializable.hpp"
#include "Os/Delegate.hpp"

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
            QueueCommon::s_queueRegistry->registerQueue(this);
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
void QueueCommon<T>::setRegistry(QueueRegistry* registry) {
    ScopeLock lock(QueueCommon::getStaticMutex());
    QueueCommon::s_queueRegistry = registry;
}
#endif

// Instantiate the common class for the implementation selected in config/OsSelection.hpp
template class QueueCommon<Delegate::Selected<Queue>::type>;

}  // namespace Os
