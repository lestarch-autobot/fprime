// ======================================================================
// \title Os/test/ut/interceptors/UtQueue.cpp
// \brief TEST ONLY: UtQueue and QueueTestInterceptor default implementations
// ======================================================================
#include <Os/test/ut/interceptors/QueueTestInterceptor.hpp>
#include <Os/test/ut/interceptors/UtQueue.hpp>

namespace Os {
namespace Test {

void QueueTestInterceptor::constructed(QueueInterface& /*real*/) {}

void QueueTestInterceptor::destructed(QueueInterface& /*real*/) {}

QueueTestInterceptor::Status QueueTestInterceptor::create(QueueInterface& real,
                                                          FwEnumStoreType id,
                                                          const Fw::ConstStringBase& name,
                                                          FwSizeType depth,
                                                          FwSizeType messageSize) {
    return real.create(id, name, depth, messageSize);
}

void QueueTestInterceptor::teardown(QueueInterface& real) {
    real.teardown();
}

QueueTestInterceptor::Status QueueTestInterceptor::send(QueueInterface& real,
                                                        const U8* buffer,
                                                        FwSizeType size,
                                                        FwQueuePriorityType priority,
                                                        BlockingType blockType) {
    return real.send(buffer, size, priority, blockType);
}

QueueTestInterceptor::Status QueueTestInterceptor::receive(QueueInterface& real,
                                                           U8* destination,
                                                           FwSizeType capacity,
                                                           BlockingType blockType,
                                                           FwSizeType& actualSize,
                                                           FwQueuePriorityType& priority) {
    return real.receive(destination, capacity, blockType, actualSize, priority);
}

FwSizeType QueueTestInterceptor::getMessagesAvailable(const QueueInterface& real) {
    return real.getMessagesAvailable();
}

FwSizeType QueueTestInterceptor::getMessageHighWaterMark(const QueueInterface& real) {
    return real.getMessageHighWaterMark();
}

QueueHandle* QueueTestInterceptor::getHandle(QueueInterface& real) {
    return real.getHandle();
}

}  // namespace Test

namespace Ut {
namespace Queue {

using Os::Test::QueueTestInterceptor;

UtQueue::UtQueue() {
    QueueTestInterceptor* interceptor = QueueTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real);
    }
}

UtQueue::~UtQueue() {
    QueueTestInterceptor* interceptor = QueueTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->destructed(this->m_real);
    }
}

UtQueue::Status UtQueue::create(FwEnumStoreType id,
                                const Fw::ConstStringBase& name,
                                FwSizeType depth,
                                FwSizeType messageSize) {
    QueueTestInterceptor* interceptor = QueueTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->create(this->m_real, id, name, depth, messageSize)
                                    : this->m_real.create(id, name, depth, messageSize);
}

void UtQueue::teardown() {
    QueueTestInterceptor* interceptor = QueueTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->teardown(this->m_real);
    } else {
        this->m_real.teardown();
    }
}

UtQueue::Status UtQueue::send(const U8* buffer, FwSizeType size, FwQueuePriorityType priority, BlockingType blockType) {
    QueueTestInterceptor* interceptor = QueueTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->send(this->m_real, buffer, size, priority, blockType)
                                    : this->m_real.send(buffer, size, priority, blockType);
}

UtQueue::Status UtQueue::receive(U8* destination,
                                 FwSizeType capacity,
                                 BlockingType blockType,
                                 FwSizeType& actualSize,
                                 FwQueuePriorityType& priority) {
    QueueTestInterceptor* interceptor = QueueTestInterceptor::getInstalled();
    return (interceptor != nullptr)
               ? interceptor->receive(this->m_real, destination, capacity, blockType, actualSize, priority)
               : this->m_real.receive(destination, capacity, blockType, actualSize, priority);
}

FwSizeType UtQueue::getMessagesAvailable() const {
    QueueTestInterceptor* interceptor = QueueTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getMessagesAvailable(this->m_real)
                                    : this->m_real.getMessagesAvailable();
}

FwSizeType UtQueue::getMessageHighWaterMark() const {
    QueueTestInterceptor* interceptor = QueueTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getMessageHighWaterMark(this->m_real)
                                    : this->m_real.getMessageHighWaterMark();
}

QueueHandle* UtQueue::getHandle() {
    QueueTestInterceptor* interceptor = QueueTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getHandle(this->m_real) : this->m_real.getHandle();
}

}  // namespace Queue
}  // namespace Ut
}  // namespace Os
