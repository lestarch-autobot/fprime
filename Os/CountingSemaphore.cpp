#include "Os/CountingSemaphore.hpp"
#include "Fw/Types/Assert.hpp"
#include "Os/Delegate.hpp"

namespace Os {
template <typename T>
CountingSemaphoreCommon<T>::CountingSemaphoreCommon(U32 initial_count) : m_delegate(initial_count) {}

template <typename T>
CountingSemaphoreCommon<T>::~CountingSemaphoreCommon() {}

template <typename T>
typename CountingSemaphoreCommon<T>::Status CountingSemaphoreCommon<T>::wait() {
    return this->m_delegate.wait();
}

template <typename T>
typename CountingSemaphoreCommon<T>::Status CountingSemaphoreCommon<T>::waitTimeout(const Fw::TimeInterval& interval) {
    return this->m_delegate.waitTimeout(interval);
}

template <typename T>
typename CountingSemaphoreCommon<T>::Status CountingSemaphoreCommon<T>::tryWait() {
    return this->m_delegate.tryWait();
}

template <typename T>
typename CountingSemaphoreCommon<T>::Status CountingSemaphoreCommon<T>::post() {
    return this->m_delegate.post();
}

template <typename T>
CountingSemaphoreHandle* CountingSemaphoreCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

// Instantiate the common class for the implementation selected in config/OsSelection.hpp
template class CountingSemaphoreCommon<Delegate::Selected<CountingSemaphore>::type>;

}  // namespace Os
