// ======================================================================
// \title Os/test/ut/implementations/UtConditionVariable.cpp
// \brief TEST ONLY: UtConditionVariable and ConditionVariableTestInterceptor default implementations
// ======================================================================
#include <Os/test/ut/implementations/UtConditionVariable.hpp>
#include <Os/test/ut/interceptors/ConditionVariableTestInterceptor.hpp>

namespace Os {
namespace Test {

void ConditionVariableTestInterceptor::constructed(ConditionVariableInterface& /*real*/) {}

void ConditionVariableTestInterceptor::destructed(ConditionVariableInterface& /*real*/) {}

ConditionVariableTestInterceptor::Status ConditionVariableTestInterceptor::pend(ConditionVariableInterface& real,
                                                                                Os::Mutex& mutex) {
    return real.pend(mutex);
}

void ConditionVariableTestInterceptor::notify(ConditionVariableInterface& real) {
    real.notify();
}

void ConditionVariableTestInterceptor::notifyAll(ConditionVariableInterface& real) {
    real.notifyAll();
}

ConditionVariableHandle* ConditionVariableTestInterceptor::getHandle(ConditionVariableInterface& real) {
    return real.getHandle();
}

}  // namespace Test

namespace Ut {
namespace Mutex {

using Os::Test::ConditionVariableTestInterceptor;

UtConditionVariable::UtConditionVariable() {
    ConditionVariableTestInterceptor* interceptor = ConditionVariableTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real);
    }
}

UtConditionVariable::~UtConditionVariable() {
    ConditionVariableTestInterceptor* interceptor = ConditionVariableTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->destructed(this->m_real);
    }
}

UtConditionVariable::Status UtConditionVariable::pend(Os::Mutex& mutex) {
    ConditionVariableTestInterceptor* interceptor = ConditionVariableTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->pend(this->m_real, mutex) : this->m_real.pend(mutex);
}

void UtConditionVariable::notify() {
    ConditionVariableTestInterceptor* interceptor = ConditionVariableTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->notify(this->m_real);
    } else {
        this->m_real.notify();
    }
}

void UtConditionVariable::notifyAll() {
    ConditionVariableTestInterceptor* interceptor = ConditionVariableTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->notifyAll(this->m_real);
    } else {
        this->m_real.notifyAll();
    }
}

ConditionVariableHandle* UtConditionVariable::getHandle() {
    ConditionVariableTestInterceptor* interceptor = ConditionVariableTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getHandle(this->m_real) : this->m_real.getHandle();
}

}  // namespace Mutex
}  // namespace Ut
}  // namespace Os
