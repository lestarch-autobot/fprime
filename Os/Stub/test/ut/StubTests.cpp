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
#include "Os/Stub/Directory.hpp"

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

// Ensure that StubDirectory performs its own checks when used without the Os::Directory delegate
TEST(Stub, DirectoryDirectChecks) {
    Os::Stub::Directory::StubDirectory directory;
    char buffer[4];
    ASSERT_EQ(directory.open("/does/not/matter", Os::DirectoryInterface::OpenMode::READ),
              Os::DirectoryInterface::Status::NOT_SUPPORTED);
    ASSERT_FALSE(directory.isOpen());
    ASSERT_EQ(directory.rewind(), Os::DirectoryInterface::Status::NOT_OPENED);
    ASSERT_EQ(directory.read(buffer, sizeof(buffer)), Os::DirectoryInterface::Status::NOT_OPENED);
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
