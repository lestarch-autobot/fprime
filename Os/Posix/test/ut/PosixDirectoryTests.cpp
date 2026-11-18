// ======================================================================
// \title Os/Posix/test/ut/PosixDirectoryTests.cpp
// \brief tests for posix implementation for Os::Directory
// ======================================================================
#include <gtest/gtest.h>
#include "Fw/Types/String.hpp"
#include "Os/Posix/Directory.hpp"
#include "Os/Posix/Task.hpp"
#include "Os/test/ut/directory/CommonTests.hpp"
#include "Os/test/ut/directory/RulesHeaders.hpp"
#include "STest/Pick/Pick.hpp"
#include "STest/Scenario/Scenario.hpp"

#include <fcntl.h>     // for ::open()
#include <sys/stat.h>  // for ::mkdir()
#include <unistd.h>    // for ::close()
#include <cstring>

namespace Os {
namespace Test {
namespace Directory {

//! Maximum number of files per test directory
//! Intentionally low to have a decent probability of having an empty directory
static const FwIndexType MAX_FILES_PER_DIRECTORY = 4;
static const std::string FILENAME_PREFIX = "test_file_";
static const std::string TEST_DIRECTORY_PATH = "./test_directory";

//! Set up function as defined by the unit test implementor
void setUp(Os::Test::Directory::Tester* tester) {
    tester->m_path = TEST_DIRECTORY_PATH;
    ::mkdir(tester->m_path.c_str(), 0777);
    // Files are named test_file_0, test_file_1, ...
    std::vector<std::string> filenames;
    // fileCount can be 0 (empty directory)
    FwSizeType fileCount = STest::Pick::lowerUpper(0, MAX_FILES_PER_DIRECTORY);
    for (FwSizeType i = 0; i < fileCount; i++) {
        tester->m_filenames.push_back(FILENAME_PREFIX + std::to_string(i));
    }
    for (auto filename : tester->m_filenames) {
        int fd = ::open((tester->m_path + "/" + filename).c_str(), O_CREAT | O_WRONLY, 0644);
        if (fd >= 0) {
            ::close(fd);
        }
    }
}

//! Tear down function as defined by the unit test implementor
void tearDown(Os::Test::Directory::Tester* tester) {
    for (auto filename : tester->m_filenames) {
        ::unlink((tester->m_path + "/" + filename).c_str());
    }
    ::rmdir(tester->m_path.c_str());
}

}  // namespace Directory
}  // namespace Test
}  // namespace Os

// ----------------------------------------------------------------------
// Posix Test Cases:
//
// The common tests are pulled in from CommonTests.cpp and run through Os::Directory. The tests below
// use PosixDirectory directly (no link-time delegate), as when Os::Directory is aliased to it.
// ----------------------------------------------------------------------

//! Fixture creating a directory holding a single file
class PosixDirectoryDirect : public ::testing::Test {
  protected:
    void SetUp() override {
        ::mkdir(PATH, 0777);
        int fd = ::open(FILE_PATH, O_CREAT | O_WRONLY, 0644);
        ASSERT_GE(fd, 0);
        ::close(fd);
    }

    void TearDown() override {
        ::unlink(FILE_PATH);
        ::rmdir(PATH);
    }

    static constexpr const char* PATH = "./test_directory_direct";
    static constexpr const char* FILE_NAME = "test_file_0";
    static constexpr const char* FILE_PATH = "./test_directory_direct/test_file_0";
    static constexpr const char* ASSERT_IN_DIRECTORY_CPP = "Assert: \".*/?Os/Posix/Directory\\.cpp:[0-9]+\"";

    Os::Posix::Directory::PosixDirectory m_directory;
};

constexpr const char* PosixDirectoryDirect::PATH;
constexpr const char* PosixDirectoryDirect::FILE_NAME;
constexpr const char* PosixDirectoryDirect::FILE_PATH;
constexpr const char* PosixDirectoryDirect::ASSERT_IN_DIRECTORY_CPP;

// Open state is tracked by the implementation without the delegate
TEST_F(PosixDirectoryDirect, OpenStateTracking) {
    ASSERT_FALSE(this->m_directory.isOpen());
    ASSERT_EQ(this->m_directory.open(PATH, Os::Directory::OpenMode::READ), Os::Directory::Status::OP_OK);
    ASSERT_TRUE(this->m_directory.isOpen());
    this->m_directory.close();
    ASSERT_FALSE(this->m_directory.isOpen());
    ASSERT_EQ(this->m_directory.open("./does_not_exist_direct", Os::Directory::OpenMode::READ),
              Os::Directory::Status::DOESNT_EXIST);
    ASSERT_FALSE(this->m_directory.isOpen());
}

// Operations on a closed directory report NOT_OPENED and leave the outputs alone
TEST_F(PosixDirectoryDirect, ClosedDirectory) {
    char buffer[8] = "abcdefg";
    Fw::String name("unchanged");
    FwSizeType count = 7;
    ASSERT_EQ(this->m_directory.rewind(), Os::Directory::Status::NOT_OPENED);
    ASSERT_EQ(this->m_directory.read(buffer, sizeof(buffer)), Os::Directory::Status::NOT_OPENED);
    ASSERT_STREQ(buffer, "abcdefg");
    ASSERT_EQ(this->m_directory.read(name), Os::Directory::Status::NOT_OPENED);
    ASSERT_STREQ(name.toChar(), "unchanged");
    ASSERT_EQ(this->m_directory.getFileCount(count), Os::Directory::Status::NOT_OPENED);
    ASSERT_EQ(count, 7);
    // Closing a closed directory is harmless
    this->m_directory.close();
    ASSERT_FALSE(this->m_directory.isOpen());
}

// Common functions defined on DirectoryInterface work on the implementation
TEST_F(PosixDirectoryDirect, CommonFunctions) {
    ASSERT_EQ(this->m_directory.open(PATH, Os::Directory::OpenMode::READ), Os::Directory::Status::OP_OK);
    FwSizeType count = 0;
    ASSERT_EQ(this->m_directory.getFileCount(count), Os::Directory::Status::OP_OK);
    ASSERT_EQ(count, 1);
    Fw::String name;
    ASSERT_EQ(this->m_directory.read(name), Os::Directory::Status::OP_OK);
    ASSERT_STREQ(name.toChar(), FILE_NAME);
    ASSERT_EQ(this->m_directory.read(name), Os::Directory::Status::NO_MORE_FILES);
    Fw::String names[2];
    Fw::ExternalArray<Fw::String> array(names, 2);
    ASSERT_EQ(this->m_directory.readDirectory(array, count), Os::Directory::Status::OP_OK);
    ASSERT_EQ(count, 1);
    ASSERT_STREQ(names[0].toChar(), FILE_NAME);
    this->m_directory.close();
}

// read() null-terminates within the supplied buffer size
TEST_F(PosixDirectoryDirect, ReadTruncatesAndTerminates) {
    ASSERT_EQ(this->m_directory.open(PATH, Os::Directory::OpenMode::READ), Os::Directory::Status::OP_OK);
    char buffer[8];
    ::memset(buffer, 'x', sizeof(buffer));
    ASSERT_EQ(this->m_directory.read(buffer, 5), Os::Directory::Status::OP_OK);
    ASSERT_STREQ(buffer, "test");
    ASSERT_EQ(buffer[5], 'x');
    // At the end of the stream the buffer is still terminated
    ::memset(buffer, 'x', sizeof(buffer));
    ASSERT_EQ(this->m_directory.read(buffer, 5), Os::Directory::Status::NO_MORE_FILES);
    ASSERT_EQ(buffer[4], '\0');
    this->m_directory.close();
}

// Invalid arguments assert in the implementation
TEST_F(PosixDirectoryDirect, InvalidArguments) {
    char buffer[8];
    ASSERT_DEATH_IF_SUPPORTED(this->m_directory.open(nullptr, Os::Directory::OpenMode::READ), ASSERT_IN_DIRECTORY_CPP);
    ASSERT_DEATH_IF_SUPPORTED(this->m_directory.open(PATH, Os::Directory::OpenMode::MAX_OPEN_MODE),
                              ASSERT_IN_DIRECTORY_CPP);
    ASSERT_EQ(this->m_directory.open(PATH, Os::Directory::OpenMode::READ), Os::Directory::Status::OP_OK);
    ASSERT_DEATH_IF_SUPPORTED(this->m_directory.read(nullptr, sizeof(buffer)), ASSERT_IN_DIRECTORY_CPP);
    ASSERT_DEATH_IF_SUPPORTED(this->m_directory.read(buffer, 0), ASSERT_IN_DIRECTORY_CPP);
    this->m_directory.close();
}

int main(int argc, char** argv) {
    STest::Random::seed();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
