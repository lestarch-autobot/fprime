#include "Os/CountingSemaphore.hpp"
#include "Fw/Types/Assert.hpp"

namespace Os {
CountingSemaphoreCommon::CountingSemaphoreCommon(U32 initial_count)
    : m_delegate(*CountingSemaphoreInterface::getDelegate(m_handle_storage, initial_count)) {}

CountingSemaphoreCommon::~CountingSemaphoreCommon() {
    m_delegate.~CountingSemaphoreInterface();
}

CountingSemaphoreCommon::Status CountingSemaphoreCommon::wait() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.wait();
}

CountingSemaphoreCommon::Status CountingSemaphoreCommon::waitTimeout(const Fw::TimeInterval& interval) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.waitTimeout(interval);
}

CountingSemaphoreCommon::Status CountingSemaphoreCommon::tryWait() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.tryWait();
}

CountingSemaphoreCommon::Status CountingSemaphoreCommon::post() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.post();
}

CountingSemaphoreHandle* CountingSemaphoreCommon::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getHandle();
}

}  // namespace Os
