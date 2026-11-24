// ======================================================================
// \title Os/Stub/test/ut/StubFileSystemTests.cpp
// \brief stub implementation for Os::FileSystemInterface testing
// This ensures the delegation of function calls happens properly
// ======================================================================
#include <gtest/gtest.h>
#include "Os/FileSystem.hpp"
#include "Os/Stub/test/FileSystem.hpp"
#include "Os/test/ut/filesystem/CommonTests.hpp"
#include "Os/test/ut/filesystem/RulesHeaders.hpp"

// Wrapper under test: Os::FileSystemCommon on the implementation named here, not the selection in
// config/OsSelection.hpp
using FileSystemUnderTest = Os::FileSystemCommon<Os::Stub::FileSystem::Test::TestFileSystem>;

using namespace Os::Stub::FileSystem::Test;

// Basic file tests
class Interface : public ::testing::Test {
  public:
    //! Setup function delegating to UT setUp function
    void SetUp() override { StaticData::data = StaticData(); }

    //! Setup function delegating to UT tearDown function
    void TearDown() override { StaticData::data = StaticData(); }
};

// Ensure that Os::FileSystem properly calls the implementation removeDirectory()
TEST_F(Interface, RemoveDirectory) {
    FileSystemUnderTest::removeDirectory("/does/not/matter");
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::REMOVE_DIR_FN);
    ASSERT_EQ(StaticData::data.lastStatus, Os::FileSystem::Status::OP_OK);
}

// Ensure that Os::FileSystem properly calls the implementation removeFile()
TEST_F(Interface, RemoveFile) {
    FileSystemUnderTest::removeFile("/does/not/matter");
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::REMOVE_FILE_FN);
    ASSERT_EQ(StaticData::data.lastStatus, Os::FileSystem::Status::OP_OK);
}

// Ensure that Os::FileSystem properly calls the implementation moveFile()
TEST_F(Interface, Rename) {
    FileSystemUnderTest::rename("/does/not/matter", "/does/not/matter");
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::RENAME_FN);
    ASSERT_EQ(StaticData::data.lastStatus, Os::FileSystem::Status::OP_OK);
}

// Ensure that Os::FileSystem properly calls the implementation changeWorkingDirectory()
TEST_F(Interface, GetWorkingDirectory) {
    char unused[1];
    FileSystemUnderTest::getWorkingDirectory(unused, 1);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::GET_CWD_FN);
    ASSERT_EQ(StaticData::data.lastStatus, Os::FileSystem::Status::OP_OK);
}

// Ensure that Os::FileSystem properly calls the implementation changeWorkingDirectory()
TEST_F(Interface, ChangeWorkingDirectory) {
    FileSystemUnderTest::changeWorkingDirectory("/does/not/matter");
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::CHANGE_CWD_FN);
    ASSERT_EQ(StaticData::data.lastStatus, Os::FileSystem::Status::OP_OK);
}

// Ensure that Os::FileSystem properly calls the implementation getFreeSpace()
TEST_F(Interface, GetFreeSpace) {
    FwSizeType totalBytes;
    FwSizeType freeBytes;
    FileSystemUnderTest::getFreeSpace("/does/not/matter", totalBytes, freeBytes);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::GET_FREESPACE_FN);
    ASSERT_EQ(StaticData::data.lastStatus, Os::FileSystem::Status::OP_OK);
}

// Ensure that Os::FileSystem properly calls the implementation getHandle()
TEST_F(Interface, GetHandle) {
    ASSERT_NE(FileSystemUnderTest::getSingleton().getHandle(), nullptr);
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::GET_HANDLE_FN);
}

// Ensure that Os::FileSystem properly calls the implementation getPathType()
TEST_F(Interface, GetPathType) {
    Os::FileSystem::PathType pathType = FileSystemUnderTest::getPathType("/does/not/matter");
    ASSERT_EQ(StaticData::data.lastCalled, StaticData::LastFn::GET_PATH_TYPE_FN);
    ASSERT_EQ(pathType, Os::FileSystem::PathType::NOT_EXIST);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
