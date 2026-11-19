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
#include <Os/Stub/RawTime.hpp>
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

// Ensure the stub RawTime implementation records, reports and copies the timer source
TEST(Stub, RawTimeImplementationSource) {
    const Os::RawTimeSource source = static_cast<Os::RawTimeSource>(Os::RAWTIME_DEFAULT + 1);
    Os::Stub::RawTime::StubRawTime implementation(source);
    ASSERT_EQ(implementation.getSource(), source);
    Os::Stub::RawTime::StubRawTime copied(implementation);
    ASSERT_EQ(copied.getSource(), source);
    Os::Stub::RawTime::StubRawTime defaulted;
    ASSERT_EQ(defaulted.getSource(), Os::RAWTIME_DEFAULT);
    defaulted = implementation;
    ASSERT_EQ(defaulted.getSource(), source);
    const Os::RawTimeInterface& interface = implementation;
    ASSERT_EQ(interface.getSource(), source);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
