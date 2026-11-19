// ======================================================================
// \title Os/QueueInterface.cpp
// \brief common function implementation for Os::QueueInterface
// ======================================================================
#include "Os/QueueInterface.hpp"
#include "Fw/Types/Assert.hpp"
#include "Fw/Types/Serializable.hpp"
#include "Os/Queue.hpp"

namespace Os {

FwSizeType QueueInterface::s_queueCount = 0;
#if FW_QUEUE_REGISTRATION
QueueRegistry* QueueInterface::s_queueRegistry = nullptr;
#endif

QueueInterface::Status QueueInterface::createChecks(FwSizeType depth, FwSizeType messageSize) const {
    FW_ASSERT(depth > 0);
    FW_ASSERT(messageSize > 0);
    // Check for previous creation call
    if (this->m_depth > 0 || this->m_size > 0) {
        return QueueInterface::Status::ALREADY_CREATED;
    }
    return QueueInterface::Status::OP_OK;
}

void QueueInterface::recordCreation(const Fw::ConstStringBase& name,
                                    FwSizeType depth,
                                    FwSizeType messageSize,
                                    Queue* queue) {
    this->m_name = name;
    this->m_depth = depth;
    this->m_size = messageSize;
    // Only the configured Os::Queue is counted and registered: an implementation wrapped by a delegate is
    // accounted for by that delegate,
    // and an object created again after a teardown is not counted or registered a second time
    if ((queue != nullptr) && (not this->m_counted)) {
        this->m_counted = true;
        ScopeLock lock(QueueInterface::getStaticMutex());
        QueueInterface::s_queueCount++;
#if FW_QUEUE_REGISTRATION
        if (QueueInterface::s_queueRegistry != nullptr) {
            QueueInterface::s_queueRegistry->registerQueue(queue);
        }
#endif
    }
}

void QueueInterface::finishTeardown() {
    this->m_name = "";
    this->m_depth = 0;
    this->m_size = 0;
}

QueueInterface::Status QueueInterface::sendChecks(const U8* buffer, FwSizeType size) const {
    FW_ASSERT(buffer != nullptr);
    // Check if initialized
    if (this->m_depth == 0 || this->m_size == 0) {
        return QueueInterface::Status::UNINITIALIZED;
    }
    // Check size before proceeding
    else if (size > this->getMessageSize()) {
        return QueueInterface::Status::SIZE_MISMATCH;
    }
    return QueueInterface::Status::OP_OK;
}

QueueInterface::Status QueueInterface::receiveChecks(const U8* destination, FwSizeType capacity) const {
    FW_ASSERT(destination != nullptr);
    // Check if initialized
    if (this->m_depth == 0 || this->m_size == 0) {
        return QueueInterface::Status::UNINITIALIZED;
    }
    // Check capacity before proceeding
    else if (capacity < this->getMessageSize()) {
        return QueueInterface::Status::SIZE_MISMATCH;
    }
    return QueueInterface::Status::OP_OK;
}

QueueInterface::Status QueueInterface::send(const Fw::LinearBufferBase& message,
                                            FwQueuePriorityType priority,
                                            QueueInterface::BlockingType blockType) {
    return this->send(message.getBuffAddr(), message.getSize(), priority, blockType);
}

QueueInterface::Status QueueInterface::receive(Fw::LinearBufferBase& destination,
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

FwSizeType QueueInterface::getDepth() const {
    return this->m_depth;
}

FwSizeType QueueInterface::getMessageSize() const {
    return this->m_size;
}

const QueueString& QueueInterface::getName() const {
    return this->m_name;
}

FwSizeType QueueInterface::getNumQueues() {
    ScopeLock lock(QueueInterface::getStaticMutex());
    return QueueInterface::s_queueCount;
}

Os::Mutex& QueueInterface::getStaticMutex() {
    static Os::Mutex s_mutex;
    return s_mutex;
}

#if FW_QUEUE_REGISTRATION
void QueueInterface::setRegistry(QueueRegistry* registry) {
    ScopeLock lock(QueueInterface::getStaticMutex());
    QueueInterface::s_queueRegistry = registry;
}
#endif

}  // namespace Os
