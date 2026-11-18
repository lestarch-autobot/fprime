// ======================================================================
// \title Os/Stub/Console.cpp
// \brief stub implementation for Os::Console
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/Stub/Console.hpp>

namespace Os {
namespace Stub {
namespace Console {

void StubConsole::writeMessage(const CHAR* message, const FwSizeType size) {
    FW_ASSERT(message != nullptr || size == 0);
}

ConsoleHandle* StubConsole::getHandle() {
    return &this->m_handle;
}

}  // namespace Console
}  // namespace Stub
}  // namespace Os
