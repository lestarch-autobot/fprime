// ======================================================================
// \title Os/DelegateCountingSemaphore.cpp
// \brief implementation of Os::DelegateCountingSemaphore (link-time delegating Os::CountingSemaphore)
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/DelegateCountingSemaphore.hpp>

namespace Os {

DelegateCountingSemaphore::DelegateCountingSemaphore(U32 initial_count)
    : m_delegate(*CountingSemaphoreInterface::getDelegate(m_handle_storage, initial_count)) {
    // m_handle_storage is placement-new storage populated by getDelegate(); do not value-initialize it
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
}

DelegateCountingSemaphore::~DelegateCountingSemaphore() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
    m_delegate.~CountingSemaphoreInterface();
}

DelegateCountingSemaphore::Status DelegateCountingSemaphore::wait() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.wait();
}

DelegateCountingSemaphore::Status DelegateCountingSemaphore::waitTimeout(const Fw::TimeInterval& interval) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.waitTimeout(interval);
}

DelegateCountingSemaphore::Status DelegateCountingSemaphore::tryWait() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.tryWait();
}

DelegateCountingSemaphore::Status DelegateCountingSemaphore::post() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.post();
}

CountingSemaphoreHandle* DelegateCountingSemaphore::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CountingSemaphoreInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getHandle();
}

}  // namespace Os
