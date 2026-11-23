// ======================================================================
// \title Os/Cpu.hpp
// \brief common function implementations for Os::Cpu
// ======================================================================
#include "Os/Cpu.hpp"
#include "Fw/Types/Assert.hpp"

namespace Os {

CpuCommon::CpuCommon() : m_delegate(*CpuInterface::getDelegate(m_handle_storage)) {}

CpuCommon::~CpuCommon() {
    m_delegate.~CpuInterface();
}

void CpuCommon::init() {
    (void)CpuCommon::getSingleton();
}

CpuCommon& CpuCommon::getSingleton() {
    static CpuCommon _singleton;
    return _singleton;
}

CpuCommon::Status CpuCommon::_getCount(FwSizeType& cpu_count) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CpuInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate._getCount(cpu_count);
}

CpuCommon::Status CpuCommon::_getTicks(Ticks& ticks, FwSizeType cpu_index) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CpuInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate._getTicks(ticks, cpu_index);
}

CpuHandle* CpuCommon::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CpuInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getHandle();
}

CpuCommon::Status CpuCommon::getCount(FwSizeType& cpu_count) {
    return CpuCommon::getSingleton()._getCount(cpu_count);
}

CpuCommon::Status CpuCommon::getTicks(Ticks& ticks, FwSizeType cpu_index) {
    return CpuCommon::getSingleton()._getTicks(ticks, cpu_index);
}
}  // namespace Os
