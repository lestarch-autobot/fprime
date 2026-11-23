// ======================================================================
// \title Os/Cpu.hpp
// \brief common function implementations for Os::Cpu
// ======================================================================
#include "Os/Cpu.hpp"
#include "Fw/Types/Assert.hpp"
#include "Os/Delegate.hpp"

namespace Os {

template <typename T>
CpuCommon<T>::CpuCommon() : m_delegate() {}

template <typename T>
CpuCommon<T>::~CpuCommon() {}

template <typename T>
void CpuCommon<T>::init() {
    (void)CpuCommon::getSingleton();
}

template <typename T>
CpuCommon<T>& CpuCommon<T>::getSingleton() {
    static CpuCommon _singleton;
    return _singleton;
}

template <typename T>
typename CpuCommon<T>::Status CpuCommon<T>::_getCount(FwSizeType& cpu_count) {
    return this->m_delegate._getCount(cpu_count);
}

template <typename T>
typename CpuCommon<T>::Status CpuCommon<T>::_getTicks(Ticks& ticks, FwSizeType cpu_index) {
    return this->m_delegate._getTicks(ticks, cpu_index);
}

template <typename T>
CpuHandle* CpuCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

template <typename T>
typename CpuCommon<T>::Status CpuCommon<T>::getCount(FwSizeType& cpu_count) {
    return CpuCommon::getSingleton()._getCount(cpu_count);
}

template <typename T>
typename CpuCommon<T>::Status CpuCommon<T>::getTicks(Ticks& ticks, FwSizeType cpu_index) {
    return CpuCommon::getSingleton()._getTicks(ticks, cpu_index);
}
// Instantiate the common class for the implementation selected in config/OsSelection.hpp
template class CpuCommon<Delegate::Selected<Cpu>::type>;

}  // namespace Os
