// ======================================================================
// \title Os/CountingSemaphore.hpp
// \brief common function definitions for Os::CountingSemaphore
// ======================================================================
#ifndef OS_COUNTING_SEMAPHORE_HPP_
#define OS_COUNTING_SEMAPHORE_HPP_
#include <Os/Interfaces/CountingSemaphoreUserInterface.hpp>
#include "Fw/Time/TimeInterval.hpp"
#include "Fw/Types/Assert.hpp"
#include "Os/Os.hpp"
#include "config/CountingSemaphoreSelection.hpp"
#ifdef BUILD_UT
#include <Os/test/ut/implementations/UtCountingSemaphore.hpp>
#endif

namespace Os {

template <typename T>
class CountingSemaphoreCommon;

#ifdef BUILD_UT
//! \brief Os::CountingSemaphore in unit test builds: wraps the implementation selected in
//! config/CountingSemaphoreSelection.hpp in the test-only UtCountingSemaphore, so that tests may intercept its calls
using CountingSemaphore = CountingSemaphoreCommon<Ut::CountingSemaphore::UtCountingSemaphore>;
#else
//! \brief Os::CountingSemaphore: the common class on the implementation selected in
//! config/CountingSemaphoreSelection.hpp
using CountingSemaphore = CountingSemaphoreCommon<Selected::CountingSemaphore>;
#endif

template <typename T>
class CountingSemaphoreCommon final : public CountingSemaphoreUserInterface {
  public:
    explicit CountingSemaphoreCommon(U32 initial_count);  //!< Constructor with initial count

    ~CountingSemaphoreCommon();  //!< Destructor

    CountingSemaphoreCommon(const CountingSemaphoreCommon& other) = delete;
    CountingSemaphoreCommon& operator=(const CountingSemaphoreCommon& other) = delete;
    CountingSemaphoreCommon(CountingSemaphoreCommon&& other) = delete;
    CountingSemaphoreCommon& operator=(CountingSemaphoreCommon&& other) = delete;

    Status wait();  //!< wait (decrement), blocking if count is zero

    Status waitTimeout(const Fw::TimeInterval& interval);  //!< wait with timeout

    Status tryWait();

    Status post();

    CountingSemaphoreHandle* getHandle();

  private:
    T m_delegate;
};

}  // namespace Os
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

}  // namespace Os

#endif  // OS_COUNTING_SEMAPHORE_HPP_
