// ======================================================================
// \title Os/test/ut/interceptors/UtMutex.cpp
// \brief TEST ONLY: Os::Mutex implementation selected in unit test builds
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/test/ut/interceptors/MutexTestInterceptor.hpp>
#include <Os/test/ut/interceptors/UtMutex.hpp>

namespace Os {
namespace Test {

MutexTestInterceptor* MutexTestInterceptor::s_installed = nullptr;

MutexTestInterceptor::MutexTestInterceptor() {
    FW_ASSERT(s_installed == nullptr);
    s_installed = this;
}

MutexTestInterceptor::~MutexTestInterceptor() {
    s_installed = nullptr;
}

MutexTestInterceptor* MutexTestInterceptor::getInstalled() {
    return s_installed;
}

void MutexTestInterceptor::constructed(MutexInterface& real) {}

void MutexTestInterceptor::destructed(MutexInterface& real) {}

MutexHandle* MutexTestInterceptor::getHandle(MutexInterface& real) {
    return real.getHandle();
}

MutexTestInterceptor::Status MutexTestInterceptor::take(MutexInterface& real) {
    return real.take();
}

MutexTestInterceptor::Status MutexTestInterceptor::release(MutexInterface& real) {
    return real.release();
}

}  // namespace Test

namespace Ut {
namespace Mutex {

using Os::Test::MutexTestInterceptor;

UtMutex::UtMutex() {
    MutexTestInterceptor* interceptor = MutexTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real);
    }
}

UtMutex::~UtMutex() {
    MutexTestInterceptor* interceptor = MutexTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->destructed(this->m_real);
    }
}

MutexHandle* UtMutex::getHandle() {
    MutexTestInterceptor* interceptor = MutexTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getHandle(this->m_real) : this->m_real.getHandle();
}

UtMutex::Status UtMutex::take() {
    MutexTestInterceptor* interceptor = MutexTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->take(this->m_real) : this->m_real.take();
}

UtMutex::Status UtMutex::release() {
    MutexTestInterceptor* interceptor = MutexTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->release(this->m_real) : this->m_real.release();
}

}  // namespace Mutex
}  // namespace Ut
}  // namespace Os
