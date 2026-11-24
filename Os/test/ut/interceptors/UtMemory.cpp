// ======================================================================
// \title Os/test/ut/interceptors/UtMemory.cpp
// \brief TEST ONLY: UtMemory and MemoryTestInterceptor default implementations
// ======================================================================
#include <Os/test/ut/interceptors/MemoryTestInterceptor.hpp>
#include <Os/test/ut/interceptors/UtMemory.hpp>

namespace Os {
namespace Test {

void MemoryTestInterceptor::constructed(MemoryInterface& /*real*/) {}

void MemoryTestInterceptor::destructed(MemoryInterface& /*real*/) {}

MemoryTestInterceptor::Status MemoryTestInterceptor::_getUsage(MemoryInterface& real, Usage& memory_usage) {
    return real._getUsage(memory_usage);
}

MemoryHandle* MemoryTestInterceptor::getHandle(MemoryInterface& real) {
    return real.getHandle();
}

}  // namespace Test

namespace Ut {
namespace Memory {

using Os::Test::MemoryTestInterceptor;

UtMemory::UtMemory() {
    MemoryTestInterceptor* interceptor = MemoryTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real);
    }
}

UtMemory::~UtMemory() {
    MemoryTestInterceptor* interceptor = MemoryTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->destructed(this->m_real);
    }
}

UtMemory::Status UtMemory::_getUsage(Usage& memory_usage) {
    MemoryTestInterceptor* interceptor = MemoryTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->_getUsage(this->m_real, memory_usage)
                                    : this->m_real._getUsage(memory_usage);
}

MemoryHandle* UtMemory::getHandle() {
    MemoryTestInterceptor* interceptor = MemoryTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getHandle(this->m_real) : this->m_real.getHandle();
}

}  // namespace Memory
}  // namespace Ut
}  // namespace Os
