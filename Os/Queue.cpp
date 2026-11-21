// ======================================================================
// \title Os/Queue.cpp
// \brief common function implementation for Os::Queue
// ======================================================================
#include "Os/Queue.hpp"
#include "Fw/Types/Assert.hpp"
#include "Fw/Types/Serializable.hpp"

namespace Os {

FwSizeType QueueCommon::s_queueCount = 0;
#if FW_QUEUE_REGISTRATION
QueueRegistry* QueueCommon::s_queueRegistry = nullptr;
#endif

QueueCommon::QueueCommon()
    : m_name(""), m_depth(0), m_size(0), m_delegate(*QueueInterface::getDelegate(m_handle_storage)) {}

QueueCommon::~QueueCommon() {
    m_delegate.~QueueInterface();
}

QueueInterface::Status QueueCommon ::create(FwEnumStoreType id,
                                            const Fw::ConstStringBase& name,
                                            FwSizeType depth,
                                            FwSizeType messageSize) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<QueueInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(depth > 0);
    FW_ASSERT(messageSize > 0);
    // Check for previous creation call
    if (this->m_depth > 0 || this->m_size > 0) {
        return QueueInterface::Status::ALREADY_CREATED;
    }
    QueueInterface::Status status = this->m_delegate.create(id, name, depth, messageSize);
    if (status == QueueInterface::Status::OP_OK) {
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

void QueueCommon::teardown() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<QueueInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.teardown();
}

QueueInterface::Status QueueCommon::send(const U8* buffer,
                                         FwSizeType size,
                                         FwQueuePriorityType priority,
                                         QueueInterface::BlockingType blockType) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<QueueInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(buffer != nullptr);
    // Check if initialized
    if (this->m_depth == 0 || this->m_size == 0) {
        return QueueInterface::Status::UNINITIALIZED;
    }
    // Check size before proceeding
    else if (size > this->getMessageSize()) {
        return QueueInterface::Status::SIZE_MISMATCH;
    }
    return this->m_delegate.send(buffer, size, priority, blockType);
}

QueueInterface::Status QueueCommon::receive(U8* destination,
                                            FwSizeType capacity,
                                            QueueInterface::BlockingType blockType,
                                            FwSizeType& actualSize,
                                            FwQueuePriorityType& priority) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<QueueInterface*>(&this->m_handle_storage[0]));
    FW_ASSERT(destination != nullptr);
    // Check if initialized
    if (this->m_depth == 0 || this->m_size == 0) {
        return QueueInterface::Status::UNINITIALIZED;
    }
    // Check capacity before proceeding
    else if (capacity < this->getMessageSize()) {
        return QueueInterface::Status::SIZE_MISMATCH;
    }
    return this->m_delegate.receive(destination, capacity, blockType, actualSize, priority);
}

FwSizeType QueueCommon::getMessagesAvailable() const {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const QueueInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getMessagesAvailable();
}

FwSizeType QueueCommon::getMessageHighWaterMark() const {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const QueueInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getMessageHighWaterMark();
}

QueueHandle* QueueCommon::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const QueueInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getHandle();
}

QueueInterface::Status QueueCommon::send(const Fw::LinearBufferBase& message,
                                         FwQueuePriorityType priority,
                                         QueueInterface::BlockingType blockType) {
    return this->send(message.getBuffAddr(), message.getSize(), priority, blockType);
}

QueueInterface::Status QueueCommon::receive(Fw::LinearBufferBase& destination,
                                            QueueInterface::BlockingType blockType,
                                            FwQueuePriorityType& priority) {
    FwSizeType actualSize = 0;
    destination.resetSer();  // Reset the buffer
    QueueInterface::Status status =
        this->receive(destination.getBuffAddrSer(), destination.getCapacity(), blockType, actualSize, priority);
    if (status == QueueInterface::Status::OP_OK) {
        Fw::SerializeStatus serializeStatus =
            destination.setBuffLen(static_cast<Fw::Serializable::SizeType>(actualSize));
        if (serializeStatus != Fw::SerializeStatus::FW_SERIALIZE_OK) {
            status = QueueInterface::Status::SIZE_MISMATCH;
        }
    }
    return status;
}

FwSizeType QueueCommon::getDepth() const {
    return this->m_depth;
}

FwSizeType QueueCommon::getMessageSize() const {
    return this->m_size;
}

const QueueString& QueueCommon::getName() const {
    return this->m_name;
}

FwSizeType QueueCommon::getNumQueues() {
    ScopeLock lock(QueueCommon::getStaticMutex());
    return QueueCommon::s_queueCount;
}

Os::Mutex& QueueCommon::getStaticMutex() {
    static Os::Mutex s_mutex;
    return s_mutex;
}

#if FW_QUEUE_REGISTRATION
void QueueCommon::setRegistry(QueueRegistry* registry) {
    ScopeLock lock(QueueCommon::getStaticMutex());
    QueueCommon::s_queueRegistry = registry;
}
#endif

}  // namespace Os
