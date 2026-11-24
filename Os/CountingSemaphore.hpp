// ======================================================================
// \title Os/CountingSemaphore.hpp
// \brief common function definitions for Os::CountingSemaphore
// ======================================================================
#ifndef OS_COUNTING_SEMAPHORE_HPP_
#define OS_COUNTING_SEMAPHORE_HPP_
#include "Fw/Time/TimeInterval.hpp"
#include "Fw/Types/Assert.hpp"
#include "Os/Os.hpp"
#include "config/OsSelection.hpp"  // defines the Os::CountingSemaphore alias and OS_COUNTING_SEMAPHORE_HEADER; do not remove

namespace Os {

class CountingSemaphoreHandle {};

//! \brief types and constants shared by users of Os::CountingSemaphore and by its implementations
struct CountingSemaphoreUserInterface {
    enum Status {
        OP_OK,                  //!< Operation was successful
        ERROR_TIMEOUT,          //!< Timeout occurred during wait
        ERROR_INVALID,          //!< Invalid semaphore or argument
        ERROR_NOT_IMPLEMENTED,  //!< Feature not implemented
        NOT_SUPPORTED,          //!< CountingSemaphore does not support operation
        ERROR_OTHER             //!< All other errors
    };
};

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
// Validate that OS_COUNTING_SEMAPHORE_HEADER was defined by config/OsSelection.hpp
#ifndef OS_COUNTING_SEMAPHORE_HEADER
#error "OS_COUNTING_SEMAPHORE_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_COUNTING_SEMAPHORE_HEADER

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
