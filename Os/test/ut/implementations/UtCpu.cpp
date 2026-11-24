// ======================================================================
// \title Os/test/ut/implementations/UtCpu.cpp
// \brief TEST ONLY: UtCpu and CpuTestInterceptor default implementations
// ======================================================================
#include <Os/test/ut/implementations/UtCpu.hpp>
#include <Os/test/ut/interceptors/CpuTestInterceptor.hpp>

namespace Os {
namespace Test {

void CpuTestInterceptor::constructed(CpuInterface& /*real*/) {}

void CpuTestInterceptor::destructed(CpuInterface& /*real*/) {}

CpuTestInterceptor::Status CpuTestInterceptor::_getCount(CpuInterface& real, FwSizeType& cpu_count) {
    return real._getCount(cpu_count);
}

CpuTestInterceptor::Status CpuTestInterceptor::_getTicks(CpuInterface& real, Ticks& ticks, FwSizeType cpu_index) {
    return real._getTicks(ticks, cpu_index);
}

CpuHandle* CpuTestInterceptor::getHandle(CpuInterface& real) {
    return real.getHandle();
}

}  // namespace Test

namespace Ut {
namespace Cpu {

using Os::Test::CpuTestInterceptor;

UtCpu::UtCpu() {
    CpuTestInterceptor* interceptor = CpuTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real);
    }
}

UtCpu::~UtCpu() {
    CpuTestInterceptor* interceptor = CpuTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->destructed(this->m_real);
    }
}

UtCpu::Status UtCpu::_getCount(FwSizeType& cpu_count) {
    CpuTestInterceptor* interceptor = CpuTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->_getCount(this->m_real, cpu_count)
                                    : this->m_real._getCount(cpu_count);
}

UtCpu::Status UtCpu::_getTicks(Ticks& ticks, FwSizeType cpu_index) {
    CpuTestInterceptor* interceptor = CpuTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->_getTicks(this->m_real, ticks, cpu_index)
                                    : this->m_real._getTicks(ticks, cpu_index);
}

CpuHandle* UtCpu::getHandle() {
    CpuTestInterceptor* interceptor = CpuTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getHandle(this->m_real) : this->m_real.getHandle();
}

}  // namespace Cpu
}  // namespace Ut
}  // namespace Os
