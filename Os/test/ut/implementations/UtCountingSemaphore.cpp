// ======================================================================
// \title Os/test/ut/implementations/UtCountingSemaphore.cpp
// \brief TEST ONLY: UtCountingSemaphore and CountingSemaphoreTestInterceptor default implementations
// ======================================================================
#include <Os/test/ut/implementations/UtCountingSemaphore.hpp>
#include <Os/test/ut/interceptors/CountingSemaphoreTestInterceptor.hpp>

namespace Os {
namespace Test {

void CountingSemaphoreTestInterceptor::constructed(CountingSemaphoreInterface& /*real*/, U32 /*initial_count*/) {}

void CountingSemaphoreTestInterceptor::destructed(CountingSemaphoreInterface& /*real*/) {}

CountingSemaphoreTestInterceptor::Status CountingSemaphoreTestInterceptor::wait(CountingSemaphoreInterface& real) {
    return real.wait();
}

CountingSemaphoreTestInterceptor::Status CountingSemaphoreTestInterceptor::waitTimeout(
    CountingSemaphoreInterface& real,
    const Fw::TimeInterval& interval) {
    return real.waitTimeout(interval);
}

CountingSemaphoreTestInterceptor::Status CountingSemaphoreTestInterceptor::tryWait(CountingSemaphoreInterface& real) {
    return real.tryWait();
}

CountingSemaphoreTestInterceptor::Status CountingSemaphoreTestInterceptor::post(CountingSemaphoreInterface& real) {
    return real.post();
}

CountingSemaphoreHandle* CountingSemaphoreTestInterceptor::getHandle(CountingSemaphoreInterface& real) {
    return real.getHandle();
}

}  // namespace Test

namespace Ut {
namespace CountingSemaphore {

using Os::Test::CountingSemaphoreTestInterceptor;

UtCountingSemaphore::UtCountingSemaphore(U32 initial_count) : m_real(initial_count) {
    CountingSemaphoreTestInterceptor* interceptor = CountingSemaphoreTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real, initial_count);
    }
}

UtCountingSemaphore::~UtCountingSemaphore() {
    CountingSemaphoreTestInterceptor* interceptor = CountingSemaphoreTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->destructed(this->m_real);
    }
}

UtCountingSemaphore::Status UtCountingSemaphore::wait() {
    CountingSemaphoreTestInterceptor* interceptor = CountingSemaphoreTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->wait(this->m_real) : this->m_real.wait();
}

UtCountingSemaphore::Status UtCountingSemaphore::waitTimeout(const Fw::TimeInterval& interval) {
    CountingSemaphoreTestInterceptor* interceptor = CountingSemaphoreTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->waitTimeout(this->m_real, interval)
                                    : this->m_real.waitTimeout(interval);
}

UtCountingSemaphore::Status UtCountingSemaphore::tryWait() {
    CountingSemaphoreTestInterceptor* interceptor = CountingSemaphoreTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->tryWait(this->m_real) : this->m_real.tryWait();
}

UtCountingSemaphore::Status UtCountingSemaphore::post() {
    CountingSemaphoreTestInterceptor* interceptor = CountingSemaphoreTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->post(this->m_real) : this->m_real.post();
}

CountingSemaphoreHandle* UtCountingSemaphore::getHandle() {
    CountingSemaphoreTestInterceptor* interceptor = CountingSemaphoreTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getHandle(this->m_real) : this->m_real.getHandle();
}

}  // namespace CountingSemaphore
}  // namespace Ut
}  // namespace Os
