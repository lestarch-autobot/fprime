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
#include <Os/Stub/Directory.hpp>
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

// StubDirectory used directly (no link-time delegate), as when Os::Directory is aliased to it
TEST(Stub, DirectoryDirect) {
    Os::Stub::Directory::StubDirectory directory;
    char buffer[8] = "abcdefg";
    Fw::String name("unchanged");
    FwSizeType count = 7;
    ASSERT_FALSE(directory.isOpen());
    ASSERT_EQ(directory.open("/does/not/matter", Os::Directory::OpenMode::READ), Os::Directory::Status::NOT_SUPPORTED);
    ASSERT_FALSE(directory.isOpen());
    ASSERT_EQ(directory.rewind(), Os::Directory::Status::NOT_OPENED);
    ASSERT_EQ(directory.read(buffer, sizeof(buffer)), Os::Directory::Status::NOT_OPENED);
    ASSERT_STREQ(buffer, "abcdefg");
    ASSERT_EQ(directory.read(name), Os::Directory::Status::NOT_OPENED);
    ASSERT_STREQ(name.toChar(), "unchanged");
    ASSERT_EQ(directory.getFileCount(count), Os::Directory::Status::NOT_OPENED);
    ASSERT_EQ(count, 7);
    directory.close();
    ASSERT_FALSE(directory.isOpen());
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
