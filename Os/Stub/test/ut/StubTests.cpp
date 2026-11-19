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
#include <Os/Stub/File.hpp>
#include <Os/Task.hpp>
#include <cstring>

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

// StubFile used directly, as when Os::File is aliased to it in config/OsSelection.hpp
TEST(Stub, FileDirect) {
    Os::Stub::File::StubFile file;
    U8 buffer[4] = {0};
    FwSizeType size = sizeof(buffer);
    FwSizeType result = 0;
    ASSERT_EQ(file.open("/does/not/matter", Os::FileInterface::Mode::OPEN_READ),
              Os::FileInterface::Status::NOT_SUPPORTED);
    ASSERT_FALSE(file.isOpen());
    ASSERT_EQ(file.read(buffer, size), Os::FileInterface::Status::NOT_OPENED);
    ASSERT_EQ(size, 0U);
    size = sizeof(buffer);
    ASSERT_EQ(file.write(buffer, size), Os::FileInterface::Status::NOT_OPENED);
    ASSERT_EQ(size, 0U);
    ASSERT_EQ(file.size(result), Os::FileInterface::Status::NOT_OPENED);
    ASSERT_EQ(file.position(result), Os::FileInterface::Status::NOT_OPENED);
    ASSERT_EQ(file.preallocate(0, 1), Os::FileInterface::Status::NOT_OPENED);
    ASSERT_EQ(file.seek_absolute(0), Os::FileInterface::Status::NOT_OPENED);
    ASSERT_EQ(file.flush(), Os::FileInterface::Status::NOT_OPENED);
    file.close();
    ASSERT_FALSE(file.isOpen());
    // The path-length check is made by the implementation's open
    char path[FileNameStringSize + 2];
    (void)::memset(path, 'a', sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';
    ASSERT_DEATH_IF_SUPPORTED((void)file.open(path, Os::FileInterface::Mode::OPEN_READ),
                              "Assert: \".*/?Os/FileInterface\\.cpp:[0-9]+\"");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
