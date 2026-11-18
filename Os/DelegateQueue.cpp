// ======================================================================
// \title Os/DelegateQueue.cpp
// \brief implementation of Os::DelegateQueue (link-time delegating Os::Queue)
// ======================================================================
#include "Os/DelegateQueue.hpp"
#include "Fw/Types/Assert.hpp"

namespace Os {

DelegateQueue::DelegateQueue() : m_delegate(*QueueInterface::getDelegate(m_handle_storage)) {
    // m_handle_storage is placement-new storage populated by getDelegate(); do not value-initialize it
    FW_ASSERT(&this->m_delegate == reinterpret_cast<QueueInterface*>(&this->m_handle_storage[0]));
}

DelegateQueue::~DelegateQueue() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<QueueInterface*>(&this->m_handle_storage[0]));
    m_delegate.~QueueInterface();
}

QueueInterface::Status DelegateQueue::create(FwEnumStoreType id,
                                             const Fw::ConstStringBase& name,
                                             FwSizeType depth,
                                             FwSizeType messageSize) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<QueueInterface*>(&this->m_handle_storage[0]));
    // The checks and bookkeeping are performed here so that every delegate behaves identically, including
    // implementations that do not perform them on their own
    QueueInterface::Status status = this->createChecks(depth, messageSize);
    if (status != QueueInterface::Status::OP_OK) {
        return status;
    }
    status = this->m_delegate.create(id, name, depth, messageSize);
    if (status == QueueInterface::Status::OP_OK) {
        this->finishCreate(*this, name, depth, messageSize);
    }
    return status;
}

void DelegateQueue::teardown() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<QueueInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.teardown();
}

QueueInterface::Status DelegateQueue::send(const U8* buffer,
                                           FwSizeType size,
                                           FwQueuePriorityType priority,
                                           QueueInterface::BlockingType blockType) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<QueueInterface*>(&this->m_handle_storage[0]));
    const QueueInterface::Status status = this->sendChecks(buffer, size);
    if (status != QueueInterface::Status::OP_OK) {
        return status;
    }
    return this->m_delegate.send(buffer, size, priority, blockType);
}

QueueInterface::Status DelegateQueue::receive(U8* destination,
                                              FwSizeType capacity,
                                              QueueInterface::BlockingType blockType,
                                              FwSizeType& actualSize,
                                              FwQueuePriorityType& priority) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<QueueInterface*>(&this->m_handle_storage[0]));
    const QueueInterface::Status status = this->receiveChecks(destination, capacity);
    if (status != QueueInterface::Status::OP_OK) {
        return status;
    }
    return this->m_delegate.receive(destination, capacity, blockType, actualSize, priority);
}

FwSizeType DelegateQueue::getMessagesAvailable() const {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const QueueInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getMessagesAvailable();
}

FwSizeType DelegateQueue::getMessageHighWaterMark() const {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const QueueInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getMessageHighWaterMark();
}

QueueHandle* DelegateQueue::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const QueueInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getHandle();
}

}  // namespace Os
