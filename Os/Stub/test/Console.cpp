// ======================================================================
// \title Os/Stub/test/Console.cpp
// \brief stub implementation for Os::Console
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/Stub/test/Console.hpp>

namespace Os {
namespace Stub {
namespace Console {
namespace Test {

StaticData StaticData::data;

TestConsole::TestConsole() {
    StaticData::data.lastCalled = StaticData::LastFn::CONSTRUCT_FN;
}

TestConsole::~TestConsole() {
    StaticData::data.lastCalled = StaticData::LastFn::DESTRUCT_FN;
}

TestConsole::TestConsole(const Os::Stub::Console::Test::TestConsole& other) {
    StaticData::data.lastCalled = StaticData::LastFn::CONSTRUCT_COPY_FN;
    StaticData::data.copyObject = &other;
    this->m_handle = other.m_handle;
}

void TestConsole::writeMessage(const CHAR* message, const FwSizeType size) {
    FW_ASSERT(message != nullptr || size == 0);
    StaticData::data.message = message;
    StaticData::data.size = size;
    StaticData::data.lastCalled = StaticData::LastFn::WRITE_FN;
}

ConsoleHandle* TestConsole::getHandle() {
    return &this->m_handle;
}

}  // namespace Test
}  // namespace Console
}  // namespace Stub
}  // namespace Os
