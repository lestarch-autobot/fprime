#include <gtest/gtest.h>
#include <Fw/Types/String.hpp>
#include <Os/Condition.hpp>
#include <Os/Console.hpp>
#include <Os/Cpu.hpp>
#include <Os/Directory.hpp>
#include <Os/File.hpp>
#include <Os/FileSystem.hpp>
#include <Os/Memory.hpp>
#include <Os/Mutex.hpp>
#include <Os/Queue.hpp>
#include <Os/RawTime.hpp>
#include <Os/Stub/Console.hpp>
#include <Os/Task.hpp>

TEST(Stub, File) {
    Os::File _;
}

TEST(Stub, Task) {
    Os::Task _;
}

TEST(Stub, Queue) {
    Os::Queue _;
}

TEST(Stub, ConditionVariable) {
    Os::ConditionVariable _;
}

TEST(Stub, Mutex) {
    Os::Mutex _;
}

TEST(Stub, Console) {
    Os::Console _;
}

// The stub console discards messages, including through the ConstStringBase overload of Os::ConsoleInterface
TEST(Stub, ConsoleWrite) {
    Os::Stub::Console::StubConsole stub_console;
    const Fw::String message("discarded");
    stub_console.writeMessage(message);
    stub_console.writeMessage("discarded", 9);
    stub_console.writeMessage(nullptr, 0);
    ASSERT_NE(stub_console.getHandle(), nullptr);
}

// A nullptr message with a non-zero size is a coding error
TEST(Stub, ConsoleWriteNull) {
    Os::Stub::Console::StubConsole stub_console;
    ASSERT_DEATH(stub_console.writeMessage(nullptr, 5), "Assert: .*Stub/Console\\.cpp");
}

TEST(Stub, FileSystem) {
    Os::FileSystem::getSingleton();
}

TEST(Stub, Directory) {
    Os::Directory _;
}

TEST(Stub, Cpu) {
    Os::Cpu::getSingleton();
}

TEST(Stub, Memory) {
    Os::Memory::getSingleton();
}

TEST(Stub, RawTime) {
    Os::RawTime _;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
