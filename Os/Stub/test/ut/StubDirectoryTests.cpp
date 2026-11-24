// ======================================================================
// \title Os/Stub/test/ut/StubDirectoryTests.cpp
// \brief interface tests for Os::Directory: the wrapper calls its implementation as expected
//
// Runs on the selected Os::Directory with a recording test interceptor in place of the implementation's behavior.
// ======================================================================
#include <gtest/gtest.h>
#include "Os/Directory.hpp"
#include "Os/test/ut/interceptors/DirectoryTestInterceptor.hpp"

namespace {

//! Records the calls Os::Directory makes on its implementation; the real directory is never touched
class Recorder : public Os::Test::DirectoryTestInterceptor {
  public:
    enum LastFn { NONE_FN, CONSTRUCT_FN, DESTRUCT_FN, OPEN_FN, REWIND_FN, READ_FN, CLOSE_FN, GET_HANDLE_FN };

    void constructed(Os::DirectoryInterface& real) override { this->lastCalled = CONSTRUCT_FN; }
    void destructed(Os::DirectoryInterface& real) override { this->lastCalled = DESTRUCT_FN; }
    Os::DirectoryHandle* getHandle(Os::DirectoryInterface& real) override {
        this->lastCalled = GET_HANDLE_FN;
        return real.getHandle();
    }
    Status open(Os::DirectoryInterface& real, const char* path, OpenMode mode) override {
        this->lastCalled = OPEN_FN;
        return Status::OP_OK;
    }
    Status rewind(Os::DirectoryInterface& real) override {
        this->lastCalled = REWIND_FN;
        return Status::OP_OK;
    }
    Status read(Os::DirectoryInterface& real, char* fileNameBuffer, FwSizeType buffSize) override {
        this->lastCalled = READ_FN;
        return Status::OP_OK;
    }
    void close(Os::DirectoryInterface& real) override { this->lastCalled = CLOSE_FN; }

    LastFn lastCalled = NONE_FN;  //!< Last function called
};

class Interface : public ::testing::Test {
  public:
    Recorder recorder;  //!< Installed for the duration of each test
};

}  // namespace

// Ensure that Os::Directory properly calls the implementation constructor
TEST_F(Interface, Construction) {
    Os::Directory directory;
    ASSERT_EQ(recorder.lastCalled, Recorder::CONSTRUCT_FN);
}

// Ensure that Os::Directory properly calls the implementation destructor
TEST_F(Interface, Destruction) {
    delete (new Os::Directory);
    ASSERT_EQ(recorder.lastCalled, Recorder::DESTRUCT_FN);
}

// Ensure that Os::Directory properly calls the implementation open()
TEST_F(Interface, Open) {
    Os::Directory directory;
    directory.open("/does/not/matter", Os::Directory::OpenMode::CREATE_IF_MISSING);
    ASSERT_EQ(recorder.lastCalled, Recorder::OPEN_FN);
}

// Ensure that Os::Directory properly calls the implementation rewind()
TEST_F(Interface, Rewind) {
    Os::Directory directory;
    directory.open("/does/not/matter", Os::Directory::OpenMode::READ);
    directory.rewind();
    ASSERT_EQ(recorder.lastCalled, Recorder::REWIND_FN);
}

// Ensure that Os::Directory properly calls the implementation read()
TEST_F(Interface, Read) {
    Os::Directory directory;
    directory.open("/does/not/matter", Os::Directory::OpenMode::READ);
    char buffer[4];
    directory.read(buffer, 4);
    ASSERT_EQ(recorder.lastCalled, Recorder::READ_FN);
}

// Ensure that Os::Directory properly calls the implementation close()
TEST_F(Interface, Close) {
    Os::Directory directory;
    directory.close();
    ASSERT_EQ(recorder.lastCalled, Recorder::CLOSE_FN);
}

// Ensure that Os::Directory properly calls the implementation getHandle()
TEST_F(Interface, GetHandle) {
    Os::Directory directory;
    ASSERT_NE(directory.getHandle(), nullptr);
    ASSERT_EQ(recorder.lastCalled, Recorder::GET_HANDLE_FN);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
