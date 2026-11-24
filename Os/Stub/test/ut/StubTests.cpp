#include <gtest/gtest.h>
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
#include <Os/Task.hpp>
#include "Os/Stub/Console.hpp"
#include "Os/Stub/Cpu.hpp"
#include "Os/Stub/Directory.hpp"
#include "Os/Stub/File.hpp"
#include "Os/Stub/FileSystem.hpp"
#include "Os/Stub/Memory.hpp"
#include "Os/Stub/Queue.hpp"
#include "Os/Stub/Task.hpp"
// Wrappers under test: Os::<S>Common on the no-op stubs, not the selection in config/OsSelection.hpp
using MemoryUnderTest = Os::MemoryCommon<Os::Stub::Memory::StubMemory>;
using CpuUnderTest = Os::CpuCommon<Os::Stub::Cpu::StubCpu>;
using DirectoryUnderTest = Os::DirectoryCommon<Os::Stub::Directory::StubDirectory>;
using FileSystemUnderTest = Os::FileSystemCommon<Os::Stub::FileSystem::StubFileSystem>;
using ConsoleUnderTest = Os::ConsoleCommon<Os::Stub::Console::StubConsole>;
using QueueUnderTest = Os::QueueCommon<Os::Stub::Queue::StubQueue>;
using TaskUnderTest = Os::TaskCommon<Os::Stub::Task::StubTask>;
using FileUnderTest = Os::FileCommon<Os::Stub::File::StubFile>;

TEST(Stub, File) {
    FileUnderTest _;
}

TEST(Stub, Task) {
    TaskUnderTest _;
}

TEST(Stub, Queue) {
    QueueUnderTest _;
}

TEST(Stub, ConditionVariable) {
    Os::ConditionVariable _;
}

TEST(Stub, Mutex) {
    Os::Mutex _;
}

TEST(Stub, Console) {
    ConsoleUnderTest _;
}

TEST(Stub, FileSystem) {
    FileSystemUnderTest::getSingleton();
}

TEST(Stub, Directory) {
    DirectoryUnderTest _;
}

TEST(Stub, Cpu) {
    CpuUnderTest::getSingleton();
}

TEST(Stub, Memory) {
    MemoryUnderTest::getSingleton();
}

TEST(Stub, RawTime) {
    Os::RawTime _;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
