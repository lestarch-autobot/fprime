// ======================================================================
// \title Os/Memory.hpp
// \brief common function implementations for Os::Memory
// ======================================================================
#include "Os/Memory.hpp"
#include "Fw/Types/Assert.hpp"
#include "Os/Delegate.hpp"

namespace Os {

template <typename T>
MemoryCommon<T>::MemoryCommon() : m_delegate() {}

template <typename T>
MemoryCommon<T>::~MemoryCommon() {}

template <typename T>
void MemoryCommon<T>::init() {
    (void)MemoryCommon::getSingleton();
}

template <typename T>
MemoryCommon<T>& MemoryCommon<T>::getSingleton() {
    static MemoryCommon _singleton;
    return _singleton;
}

template <typename T>
typename MemoryCommon<T>::Status MemoryCommon<T>::_getUsage(Os::MemoryCommon<T>::Usage& memory_usage) {
    return this->m_delegate._getUsage(memory_usage);
}

template <typename T>
typename MemoryCommon<T>::Status MemoryCommon<T>::getUsage(Os::MemoryCommon<T>::Usage& memory_usage) {
    return MemoryCommon::getSingleton()._getUsage(memory_usage);
}

template <typename T>
MemoryHandle* MemoryCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}
// Instantiate the common class for the implementation selected in config/OsSelection.hpp
template class MemoryCommon<Delegate::Selected<Memory>::type>;

}  // namespace Os
