// ======================================================================
// \title Os/Stub/test/ut/StubFileSystemTests.cpp
// \brief interface tests for Os::FileSystem: the wrapper calls its implementation as expected
//
// Runs on the selected Os::FileSystem with a recording test interceptor in place of the implementation's behavior.
// ======================================================================
#include <gtest/gtest.h>
#include "Os/FileSystem.hpp"
#include "Os/test/ut/interceptors/FileSystemTestInterceptor.hpp"

namespace {

//! Records the calls Os::FileSystem makes on its implementation; the real file system is never touched
class Recorder : public Os::Test::FileSystemTestInterceptor {
  public:
    enum LastFn {
        NONE_FN,
        REMOVE_DIR_FN,
        REMOVE_FILE_FN,
        RENAME_FN,
        GET_CWD_FN,
        CHANGE_CWD_FN,
        GET_FREESPACE_FN,
        GET_PATH_TYPE_FN,
        GET_HANDLE_FN
    };

    Os::FileSystemHandle* getHandle(Os::FileSystemInterface& real) override {
        this->lastCalled = GET_HANDLE_FN;
        return real.getHandle();
    }
    Status _removeDirectory(Os::FileSystemInterface& real, const char* path) override {
        this->lastCalled = REMOVE_DIR_FN;
        return this->status_out;
    }
    Status _removeFile(Os::FileSystemInterface& real, const char* path) override {
        this->lastCalled = REMOVE_FILE_FN;
        return this->status_out;
    }
    Status _rename(Os::FileSystemInterface& real, const char* sourcePath, const char* destPath) override {
        this->lastCalled = RENAME_FN;
        return this->status_out;
    }
    Status _getFreeSpace(Os::FileSystemInterface& real,
                         const char* path,
                         FwSizeType& totalBytes,
                         FwSizeType& freeBytes) override {
        this->lastCalled = GET_FREESPACE_FN;
        return this->status_out;
    }
    Status _getPathType(Os::FileSystemInterface& real, const char* path, PathType& pathType) override {
        this->lastCalled = GET_PATH_TYPE_FN;
        pathType = PathType::NOT_EXIST;
        return this->status_out;
    }
    Status _getWorkingDirectory(Os::FileSystemInterface& real, char* path, FwSizeType bufferSize) override {
        this->lastCalled = GET_CWD_FN;
        return this->status_out;
    }
    Status _changeWorkingDirectory(Os::FileSystemInterface& real, const char* path) override {
        this->lastCalled = CHANGE_CWD_FN;
        return this->status_out;
    }

    LastFn lastCalled = NONE_FN;        //!< Last function called
    Status status_out = Status::OP_OK;  //!< Status to return
};

class Interface : public ::testing::Test {
  public:
    Recorder recorder;  //!< Installed for the duration of each test
};

}  // namespace

// Ensure that Os::FileSystem properly calls the implementation removeDirectory()
TEST_F(Interface, RemoveDirectory) {
    recorder.status_out = Os::FileSystem::Status::NOT_EMPTY;
    ASSERT_EQ(Os::FileSystem::removeDirectory("/does/not/matter"), Os::FileSystem::Status::NOT_EMPTY);
    ASSERT_EQ(recorder.lastCalled, Recorder::REMOVE_DIR_FN);
}

// Ensure that Os::FileSystem properly calls the implementation removeFile()
TEST_F(Interface, RemoveFile) {
    recorder.status_out = Os::FileSystem::Status::NO_PERMISSION;
    ASSERT_EQ(Os::FileSystem::removeFile("/does/not/matter"), Os::FileSystem::Status::NO_PERMISSION);
    ASSERT_EQ(recorder.lastCalled, Recorder::REMOVE_FILE_FN);
}

// Ensure that Os::FileSystem properly calls the implementation rename()
TEST_F(Interface, Rename) {
    recorder.status_out = Os::FileSystem::Status::NO_PERMISSION;
    ASSERT_EQ(Os::FileSystem::rename("/does/not/matter", "/does/not/matter"), Os::FileSystem::Status::NO_PERMISSION);
    ASSERT_EQ(recorder.lastCalled, Recorder::RENAME_FN);
}

// Ensure that Os::FileSystem properly calls the implementation getWorkingDirectory()
TEST_F(Interface, GetWorkingDirectory) {
    char unused[1];
    recorder.status_out = Os::FileSystem::Status::NO_PERMISSION;
    ASSERT_EQ(Os::FileSystem::getWorkingDirectory(unused, 1), Os::FileSystem::Status::NO_PERMISSION);
    ASSERT_EQ(recorder.lastCalled, Recorder::GET_CWD_FN);
}

// Ensure that Os::FileSystem properly calls the implementation changeWorkingDirectory()
TEST_F(Interface, ChangeWorkingDirectory) {
    recorder.status_out = Os::FileSystem::Status::NO_PERMISSION;
    ASSERT_EQ(Os::FileSystem::changeWorkingDirectory("/does/not/matter"), Os::FileSystem::Status::NO_PERMISSION);
    ASSERT_EQ(recorder.lastCalled, Recorder::CHANGE_CWD_FN);
}

// Ensure that Os::FileSystem properly calls the implementation getFreeSpace()
TEST_F(Interface, GetFreeSpace) {
    FwSizeType totalBytes;
    FwSizeType freeBytes;
    recorder.status_out = Os::FileSystem::Status::NO_PERMISSION;
    ASSERT_EQ(Os::FileSystem::getFreeSpace("/does/not/matter", totalBytes, freeBytes),
              Os::FileSystem::Status::NO_PERMISSION);
    ASSERT_EQ(recorder.lastCalled, Recorder::GET_FREESPACE_FN);
}

// Ensure that Os::FileSystem properly calls the implementation getHandle()
TEST_F(Interface, GetHandle) {
    ASSERT_NE(Os::FileSystem::getSingleton().getHandle(), nullptr);
    ASSERT_EQ(recorder.lastCalled, Recorder::GET_HANDLE_FN);
}

// Ensure that Os::FileSystem properly calls the implementation getPathType()
TEST_F(Interface, GetPathType) {
    Os::FileSystem::PathType pathType = Os::FileSystem::getPathType("/does/not/matter");
    ASSERT_EQ(recorder.lastCalled, Recorder::GET_PATH_TYPE_FN);
    ASSERT_EQ(pathType, Os::FileSystem::PathType::NOT_EXIST);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
