// ======================================================================
// \title Os/DelegateCpu.cpp
// \brief common function implementation for Os::CpuInterface and Os::DelegateCpu
// ======================================================================
#include "Os/DelegateCpu.hpp"
#include <type_traits>
#include "Fw/Types/Assert.hpp"
#include "Os/Cpu.hpp"

namespace Os {

DelegateCpu::DelegateCpu() : m_delegate(*CpuInterface::getDelegate(m_handle_storage)) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CpuInterface*>(&this->m_handle_storage[0]));
}

DelegateCpu::~DelegateCpu() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CpuInterface*>(&this->m_handle_storage[0]));
    m_delegate.~CpuInterface();
}

DelegateCpu::Status DelegateCpu::_getCount(FwSizeType& cpu_count) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CpuInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate._getCount(cpu_count);
}

DelegateCpu::Status DelegateCpu::_getTicks(Ticks& ticks, FwSizeType cpu_index) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CpuInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate._getTicks(ticks, cpu_index);
}

CpuHandle* DelegateCpu::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<CpuInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getHandle();
}

// ------------------------------------------------------------
// CpuInterface common implementations
// Built on the configured Os::Cpu and the pure virtual functions.
// Located here (not CpuInterface.cpp) to keep Cpu implementation
// code in one translation unit.
// ------------------------------------------------------------

static_assert(std::is_convertible<Os::Cpu*, CpuInterface*>::value,
              "Os::Cpu must publicly derive from Os::CpuInterface");
static_assert(std::is_default_constructible<Os::Cpu>::value, "Os::Cpu must be default constructible");

void CpuInterface::init() {
    (void)CpuInterface::getSingleton();
}

Os::Cpu& CpuInterface::getSingleton() {
    static Os::Cpu _singleton;
    return _singleton;
}

CpuInterface::Status CpuInterface::getCount(FwSizeType& cpu_count) {
    return CpuInterface::getSingleton()._getCount(cpu_count);
}

CpuInterface::Status CpuInterface::getTicks(Ticks& ticks, FwSizeType cpu_index) {
    return CpuInterface::getSingleton()._getTicks(ticks, cpu_index);
}
}  // namespace Os
