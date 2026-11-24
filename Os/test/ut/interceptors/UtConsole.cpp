// ======================================================================
// \title Os/test/ut/interceptors/UtConsole.cpp
// \brief TEST ONLY: UtConsole and ConsoleTestInterceptor default implementations
// ======================================================================
#include <Os/test/ut/interceptors/ConsoleTestInterceptor.hpp>
#include <Os/test/ut/interceptors/UtConsole.hpp>

namespace Os {
namespace Test {

void ConsoleTestInterceptor::constructed(ConsoleInterface& /*real*/) {}

void ConsoleTestInterceptor::destructed(ConsoleInterface& /*real*/) {}

void ConsoleTestInterceptor::copyConstructed(ConsoleInterface& /*real*/, const ConsoleInterface& /*other*/) {}

void ConsoleTestInterceptor::copyAssigned(ConsoleInterface& /*real*/, const ConsoleInterface& /*other*/) {}

void ConsoleTestInterceptor::writeMessage(ConsoleInterface& real, const CHAR* message, const FwSizeType size) {
    real.writeMessage(message, size);
}

ConsoleHandle* ConsoleTestInterceptor::getHandle(ConsoleInterface& real) {
    return real.getHandle();
}

}  // namespace Test

namespace Ut {
namespace Console {

using Os::Test::ConsoleTestInterceptor;

UtConsole::UtConsole() {
    ConsoleTestInterceptor* interceptor = ConsoleTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real);
    }
}

UtConsole::UtConsole(const UtConsole& other) : m_real(other.m_real) {
    ConsoleTestInterceptor* interceptor = ConsoleTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->copyConstructed(this->m_real, other.m_real);
    }
}

UtConsole& UtConsole::operator=(const UtConsole& other) {
    if (this != &other) {
        this->m_real = other.m_real;
    }
    ConsoleTestInterceptor* interceptor = ConsoleTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->copyAssigned(this->m_real, other.m_real);
    }
    return *this;
}

UtConsole::~UtConsole() {
    ConsoleTestInterceptor* interceptor = ConsoleTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->destructed(this->m_real);
    }
}

void UtConsole::writeMessage(const CHAR* message, const FwSizeType size) {
    ConsoleTestInterceptor* interceptor = ConsoleTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->writeMessage(this->m_real, message, size);
    } else {
        this->m_real.writeMessage(message, size);
    }
}

ConsoleHandle* UtConsole::getHandle() {
    ConsoleTestInterceptor* interceptor = ConsoleTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getHandle(this->m_real) : this->m_real.getHandle();
}

}  // namespace Console
}  // namespace Ut
}  // namespace Os
