// ======================================================================
// \title Os/Stub/Queue.cpp
// \brief stub implementation for Os::Queue
// ======================================================================
#include "Queue.hpp"

namespace Os {
namespace Stub {
namespace Queue {

QueueInterface::Status StubQueue::create(FwEnumStoreType id,
                                         const Fw::ConstStringBase& name,
                                         FwSizeType depth,
                                         FwSizeType messageSize) {
    // Common create checks: non-zero depth and message size, and not already created
    const QueueInterface::Status checkStatus = this->createChecks(depth, messageSize);
    if (checkStatus != QueueInterface::Status::OP_OK) {
        return checkStatus;
    }
    return QueueInterface::Status::UNKNOWN_ERROR;
}

void StubQueue::teardown() {
    this->finishTeardown();
}

QueueInterface::Status StubQueue::send(const U8* buffer,
                                       FwSizeType size,
                                       FwQueuePriorityType priority,
                                       QueueInterface::BlockingType blockType) {
    // Common send checks: non-null buffer, queue created, and message fits
    const QueueInterface::Status checkStatus = this->sendChecks(buffer, size);
    if (checkStatus != QueueInterface::Status::OP_OK) {
        return checkStatus;
    }
    return QueueInterface::Status::UNINITIALIZED;
}

QueueInterface::Status StubQueue::receive(U8* destination,
                                          FwSizeType capacity,
                                          QueueInterface::BlockingType blockType,
                                          FwSizeType& actualSize,
                                          FwQueuePriorityType& priority) {
    // Common receive checks: non-null destination, queue created, and capacity holds any message
    const QueueInterface::Status checkStatus = this->receiveChecks(destination, capacity);
    if (checkStatus != QueueInterface::Status::OP_OK) {
        return checkStatus;
    }
    return QueueInterface::Status::UNINITIALIZED;
}

FwSizeType StubQueue::getMessagesAvailable() const {
    return 0;
}

FwSizeType StubQueue::getMessageHighWaterMark() const {
    return 0;
}

QueueHandle* StubQueue::getHandle() {
    return &this->m_handle;
}

}  // namespace Queue
}  // namespace Stub
}  // namespace Os
