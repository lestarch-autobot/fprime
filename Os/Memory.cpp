// ======================================================================
// \title Os/Memory.hpp
// \brief common function implementations for Os::Memory
// ======================================================================
#include "Os/Memory.hpp"
#include "Fw/Types/Assert.hpp"

namespace Os {

MemoryCommon::MemoryCommon() : m_delegate(*MemoryInterface::getDelegate(m_handle_storage)) {}

MemoryCommon::~MemoryCommon() {
    m_delegate.~MemoryInterface();
}

void MemoryCommon::init() {
    (void)MemoryCommon::getSingleton();
}

MemoryCommon& MemoryCommon::getSingleton() {
    static MemoryCommon _singleton;
    return _singleton;
}

MemoryCommon::Status MemoryCommon::_getUsage(Os::MemoryCommon::Usage& memory_usage) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<MemoryInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate._getUsage(memory_usage);
}

MemoryCommon::Status MemoryCommon::getUsage(Os::MemoryCommon::Usage& memory_usage) {
    return MemoryCommon::getSingleton()._getUsage(memory_usage);
}

MemoryHandle* MemoryCommon::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<MemoryInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getHandle();
}
}  // namespace Os
