// ======================================================================
// \title Os/Stub/test/ut/StubFileTests.cpp
// \brief tests using stub implementation for Os::File interface testing
// ======================================================================
#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include "Os/File.hpp"
#include "Os/Os.hpp"
#include "Os/test/ut/file/CommonTests.hpp"
#include "Os/test/ut/file/RulesHeaders.hpp"
#include "Os/test/ut/interceptors/FileTestInterceptor.hpp"

namespace Os {
namespace Stub {
namespace File {
namespace Test {

//! Data recorded by, and injected into, the interceptor below.
//!/
struct StaticData {
    //! Enumeration of last function called
    //!
    enum LastFn {
        NONE_FN,
        CONSTRUCT_FN,
        DESTRUCT_FN,
        OPEN_FN,
        CLOSE_FN,
        SIZE_FN,
        POSITION_FN,
        PREALLOCATE_FN,
        SEEK_FN,
        FLUSH_FN,
        READ_FN,
        WRITE_FN
    };

    //! Last function called
    LastFn lastCalled = NONE_FN;
    //! Path of last open call
    const char* openPath = nullptr;
    //! Mode of last open call
    Os::File::Mode openMode = Os::File::MAX_OPEN_MODE;
    //! Overwrite of last open call
    Os::File::OverwriteType openOverwrite = Os::File::OverwriteType::NO_OVERWRITE;
    //! Offset of last preallocate call
    FwSizeType preallocateOffset = std::numeric_limits<FwSizeType>::max();
    //! Length of last preallocate call
    FwSizeType preallocateLength = std::numeric_limits<FwSizeType>::max();
    //! Offset of last seek call
    FwSizeType seekOffset = std::numeric_limits<FwSizeType>::max();
    //! Absolute of last seek call
    Os::File::SeekType seekType = Os::File::SeekType::ABSOLUTE;
    //! Buffer of last read call
    U8* readBuffer = nullptr;
    //! Size of last read call
    FwSizeType readSize = std::numeric_limits<FwSizeType>::max();
    //! Wait of last read call
    Os::File::WaitType readWait = Os::File::WaitType::NO_WAIT;
    //! Buffer of last write call
    const void* writeBuffer = nullptr;
    //! Size of last write call
    FwSizeType writeSize = std::numeric_limits<FwSizeType>::max();
    //! Wait of last write call
    Os::File::WaitType writeWait = Os::File::WaitType::NO_WAIT;

    //! File pointer
    FwSizeType pointer = 0;

    //! Status to return from open
    Os::File::Status openStatus = Os::File::Status::OP_OK;
    //! Status to return from size
    Os::File::Status sizeStatus = Os::File::Status::OP_OK;
    //! Status to return from position
    Os::File::Status positionStatus = Os::File::Status::OP_OK;
    //! Status to return from preallocate
    Os::File::Status preallocateStatus = Os::File::Status::OP_OK;
    //! Status to return from seek
    Os::File::Status seekStatus = Os::File::Status::OP_OK;
    //! Status to return from flush
    Os::File::Status flushStatus = Os::File::Status::OP_OK;
    //! Status to return from read
    Os::File::Status readStatus = Os::File::Status::OP_OK;
    //! Status to return from write
    Os::File::Status writeStatus = Os::File::Status::OP_OK;

    //! Return of next size call
    FwSizeType sizeResult = std::numeric_limits<FwSizeType>::max();
    //! Return of next position call
    FwSizeType positionResult = 0;
    //! Result of next read call
    U8* readResult = nullptr;
    //! Size result of next read data
    FwSizeType readResultSize = std::numeric_limits<FwSizeType>::max();
    //! Size result of next read call
    FwSizeType readSizeResult = std::numeric_limits<FwSizeType>::max();
    //! Result holding buffer of next write call
    U8* writeResult = nullptr;
    //! Size result of next write data
    FwSizeType writeResultSize = std::numeric_limits<FwSizeType>::max();
    //! Size result of next read call
    FwSizeType writeSizeResult = std::numeric_limits<FwSizeType>::max();

    // Singleton data
    static StaticData data;

    //! Set next status to return
    static void setNextStatus(Os::File::Status status);

    //! Set next size to result
    static void setSizeResult(FwSizeType size);

    //! Set next position to result
    static void setPositionResult(FwSizeType position);

    //! Set next read result
    static void setReadResult(U8* buffer, FwSizeType size);

    //! Set next read size result
    static void setReadSize(FwSizeType size);

    //! Set next write result
    static void setWriteResult(U8* buffer, FwSizeType size);

    //! Set next write size result
    static void setWriteSize(FwSizeType size);
};

//! \brief TEST ONLY: interceptor that replaces every Os::File call with tracked, injectable behavior
//!
//! While an instance exists, no Os::File call reaches the real implementation: calls are recorded in, and answered
//! from, `StaticData::data`. Subclass and override to alter individual calls.
class TestFileInterceptor : public Os::Test::FileTestInterceptor {
  public:
    void constructed(FileInterface& real) override;
    void destructed(FileInterface& real) override;
    Status open(FileInterface& real, const char* path, Mode mode, OverwriteType overwrite) override;
    void close(FileInterface& real) override;
    Status size(FileInterface& real, FwSizeType& size_result) override;
    Status position(FileInterface& real, FwSizeType& position_result) override;
    Status preallocate(FileInterface& real, FwSizeType offset, FwSizeType length) override;
    Status seek(FileInterface& real, FwSignedSizeType offset, SeekType seekType) override;
    Status flush(FileInterface& real) override;
    Status read(FileInterface& real, U8* buffer, FwSizeType& size, WaitType wait) override;
    Status write(FileInterface& real, const U8* buffer, FwSizeType& size, WaitType wait) override;
};

StaticData StaticData::data;

void StaticData::setNextStatus(Os::File::Status status) {
    StaticData::data.openStatus = status;
    StaticData::data.sizeStatus = status;
    StaticData::data.positionStatus = status;
    StaticData::data.preallocateStatus = status;
    StaticData::data.seekStatus = status;
    StaticData::data.flushStatus = status;
    StaticData::data.readStatus = status;
    StaticData::data.writeStatus = status;
}

void StaticData::setSizeResult(FwSizeType size) {
    StaticData::data.sizeResult = size;
}

void StaticData::setPositionResult(FwSizeType position) {
    StaticData::data.positionResult = position;
}

void StaticData::setReadResult(U8* buffer, FwSizeType size) {
    StaticData::data.readResult = buffer;
    StaticData::data.readResultSize = size;
}

void StaticData::setReadSize(FwSizeType size) {
    StaticData::data.readSizeResult = size;
}

void StaticData::setWriteResult(U8* buffer, FwSizeType size) {
    StaticData::data.writeResult = buffer;
    StaticData::data.writeResultSize = size;
}

void StaticData::setWriteSize(FwSizeType size) {
    StaticData::data.writeSizeResult = size;
}

void TestFileInterceptor::constructed(FileInterface& /*real*/) {
    StaticData::data.lastCalled = StaticData::CONSTRUCT_FN;
}

void TestFileInterceptor::destructed(FileInterface& /*real*/) {
    StaticData::data.lastCalled = StaticData::DESTRUCT_FN;
}

FileInterface::Status TestFileInterceptor::open(FileInterface& /*real*/,
                                                const char* filepath,
                                                Mode open_mode,
                                                OverwriteType overwrite) {
    StaticData::data.openPath = filepath;
    StaticData::data.openMode = open_mode;
    StaticData::data.openOverwrite = overwrite;
    StaticData::data.lastCalled = StaticData::OPEN_FN;
    StaticData::data.pointer = 0;
    StaticData::data.positionResult = StaticData::data.pointer;
    return StaticData::data.openStatus;
}

void TestFileInterceptor::close(FileInterface& /*real*/) {
    StaticData::data.lastCalled = StaticData::CLOSE_FN;
}

FileInterface::Status TestFileInterceptor::size(FileInterface& /*real*/, FwSizeType& size_result) {
    StaticData::data.lastCalled = StaticData::SIZE_FN;
    size_result = StaticData::data.sizeResult;
    return StaticData::data.sizeStatus;
}

FileInterface::Status TestFileInterceptor::position(FileInterface& /*real*/, FwSizeType& position_result) {
    StaticData::data.lastCalled = StaticData::POSITION_FN;
    position_result = StaticData::data.positionResult;
    return StaticData::data.positionStatus;
}

FileInterface::Status TestFileInterceptor::preallocate(FileInterface& /*real*/, FwSizeType offset, FwSizeType length) {
    StaticData::data.preallocateOffset = offset;
    StaticData::data.preallocateLength = length;
    StaticData::data.lastCalled = StaticData::PREALLOCATE_FN;
    return StaticData::data.preallocateStatus;
}

FileInterface::Status TestFileInterceptor::seek(FileInterface& /*real*/, FwSignedSizeType offset, SeekType seekType) {
    StaticData::data.seekOffset = offset;
    StaticData::data.seekType = seekType;
    StaticData::data.lastCalled = StaticData::SEEK_FN;
    StaticData::data.pointer = offset;
    StaticData::data.positionResult = StaticData::data.pointer;

    return StaticData::data.seekStatus;
}

FileInterface::Status TestFileInterceptor::flush(FileInterface& /*real*/) {
    StaticData::data.lastCalled = StaticData::FLUSH_FN;
    return StaticData::data.flushStatus;
}

FileInterface::Status TestFileInterceptor::read(FileInterface& /*real*/, U8* buffer, FwSizeType& size, WaitType wait) {
    StaticData::data.readBuffer = buffer;
    StaticData::data.readSize = size;
    StaticData::data.readWait = wait;
    StaticData::data.lastCalled = StaticData::READ_FN;
    // Copy read data if set
    if (nullptr != StaticData::data.readResult) {
        size = std::min(size, StaticData::data.readResultSize - StaticData::data.pointer);
        (void)::memcpy(buffer, StaticData::data.readResult + StaticData::data.pointer, static_cast<size_t>(size));
        StaticData::data.pointer += size;
        StaticData::data.positionResult = StaticData::data.pointer;
    } else {
        size = StaticData::data.readSizeResult;
    }
    return StaticData::data.readStatus;
}

FileInterface::Status TestFileInterceptor::write(FileInterface& /*real*/,
                                                 const U8* buffer,
                                                 FwSizeType& size,
                                                 WaitType wait) {
    StaticData::data.writeBuffer = buffer;
    StaticData::data.writeSize = size;
    StaticData::data.writeWait = wait;
    StaticData::data.lastCalled = StaticData::WRITE_FN;
    // Copy read data if set
    if (nullptr != StaticData::data.writeResult) {
        size = std::min(size, StaticData::data.writeResultSize - StaticData::data.pointer);
        (void)::memcpy(StaticData::data.writeResult + StaticData::data.pointer, buffer, static_cast<size_t>(size));
        StaticData::data.pointer += size;
        StaticData::data.positionResult = StaticData::data.pointer;
    } else {
        size = StaticData::data.writeSizeResult;
    }
    return StaticData::data.writeStatus;
}

}  // namespace Test
}  // namespace File
}  // namespace Stub
}  // namespace Os

namespace Os {
namespace Test {
namespace FileTest {

//! Set up for the test ensures that the test can run at all
//!
void setUp(bool requires_io) {
    if (requires_io) {
        GTEST_SKIP() << "Cannot run tests requiring functional i/o";
    }
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OP_OK);
    Os::Stub::File::Test::StaticData::setPositionResult(0);
}
//! Installed for the whole process: every Os::File call is answered from Os::Stub::File::Test::StaticData
Os::Stub::File::Test::TestFileInterceptor INTERCEPTOR;

std::vector<std::shared_ptr<const std::string> > FILES;
//! Tear down for the tests cleans up the test file used
//!
void tearDown() {}

class StubsTester : public Tester {
    //! Check if the test file exists.
    //! \return true if it exists, false otherwise.
    //!
    bool exists(const std::string& filename) const override {
        for (size_t i = 0; i < FILES.size(); i++) {
            if (filename == *FILES.at(i)) {
                return true;
            }
        }
        return false;
    }

    //! Get a filename, randomly if random is true, otherwise use a basic filename.
    //! \param random: true if filename should be random, false if predictable
    //! \return: filename to use for testing
    //!
    std::shared_ptr<const std::string> get_filename(bool random) const override {
        std::shared_ptr<const std::string> filename =
            std::shared_ptr<const std::string>(new std::string("DOES-NOT-MATTER"), std::default_delete<std::string>());
        FILES.push_back(filename);
        return filename;
    }

    //! Posix tester is fully functional
    //! \return true
    //!
    bool functional() const override { return false; }
};

std::unique_ptr<Os::Test::FileTest::Tester> get_tester_implementation() {
    return std::unique_ptr<Os::Test::FileTest::Tester>(new Os::Test::FileTest::StubsTester());
}

}  // namespace FileTest
}  // namespace Test
}  // namespace Os

// Ensure that Os::File properly routes constructor calls to the `constructInternal` function.
TEST_F(Interface, Construction) {
    Os::File file;
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.lastCalled, Os::Stub::File::Test::StaticData::CONSTRUCT_FN);
}

// Ensure that Os::File properly routes destructor calls to the `destructInternal` function.
TEST_F(Interface, Destruction) {
    delete (new Os::File);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.lastCalled, Os::Stub::File::Test::StaticData::DESTRUCT_FN);
}

// Ensure that Os::File properly routes open calls to the `openInternal` function.
TEST_F(Interface, Open) {
    const char* path = "/does/not/matter";
    Os::File file;
    ASSERT_EQ(file.open(path, Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE), Os::File::Status::OP_OK);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.lastCalled, Os::Stub::File::Test::StaticData::OPEN_FN);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.openPath, path);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.openMode, Os::File::OPEN_CREATE);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.openOverwrite, Os::File::OverwriteType::OVERWRITE);
}

// Ensure that Os::File properly routes close calls to the `closeInternal` function.
TEST_F(Interface, Close) {
    Os::File file;
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OP_OK);
    ASSERT_EQ(file.open("/does/not/matter", Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE),
              Os::File::OP_OK);
    file.close();
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.lastCalled, Os::Stub::File::Test::StaticData::CLOSE_FN);
}

// Ensure that Os::File properly routes close calls to the `sizeInternal` function.
TEST_F(Interface, Size) {
    FwSizeType sizeResult = std::numeric_limits<FwSizeType>::max();
    Os::File file;
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OP_OK);
    ASSERT_EQ(file.open("/does/not/matter", Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE),
              Os::File::OP_OK);
    Os::Stub::File::Test::StaticData::setSizeResult(30);
    ASSERT_EQ(file.size(sizeResult), Os::File::OP_OK);
    ASSERT_EQ(sizeResult, 30);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.lastCalled, Os::Stub::File::Test::StaticData::SIZE_FN);
}

// Ensure that Os::File properly routes close calls to the `positionInternal` function.
TEST_F(Interface, Position) {
    FwSizeType positionResult = std::numeric_limits<FwSizeType>::max();
    Os::File file;
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OP_OK);
    ASSERT_EQ(file.open("/does/not/matter", Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE),
              Os::File::OP_OK);
    Os::Stub::File::Test::StaticData::setPositionResult(50);
    ASSERT_EQ(file.position(positionResult), Os::File::OP_OK);
    ASSERT_EQ(positionResult, 50);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.lastCalled, Os::Stub::File::Test::StaticData::POSITION_FN);
}

// Ensure that Os::File properly routes preallocate calls to the `preallocateInternal` function.
TEST_F(Interface, Preallocate) {
    Os::File file;
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OP_OK);
    ASSERT_EQ(file.open("/does/not/matter", Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE),
              Os::File::OP_OK);
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OTHER_ERROR);
    ASSERT_EQ(file.preallocate(0, 0), Os::File::Status::OTHER_ERROR);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.lastCalled, Os::Stub::File::Test::StaticData::PREALLOCATE_FN);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.preallocateOffset, 0);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.preallocateLength, 0);
}

// Ensure that Os::File properly routes seek calls to the `seekInternal` function.
TEST_F(Interface, Seek) {
    Os::File file;
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OP_OK);
    ASSERT_EQ(file.open("/does/not/matter", Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE),
              Os::File::OP_OK);
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OTHER_ERROR);
    ASSERT_EQ(file.seek(0, Os::File::SeekType::ABSOLUTE), Os::File::Status::OTHER_ERROR);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.lastCalled, Os::Stub::File::Test::StaticData::SEEK_FN);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.seekOffset, 0);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.seekType, Os::File::SeekType::ABSOLUTE);
}

// Ensure that Os::File properly routes flush calls to the `flushInternal` function.
TEST_F(Interface, Flush) {
    Os::File file;
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OP_OK);
    ASSERT_EQ(file.open("/does/not/matter", Os::File::OPEN_CREATE, Os::File::OverwriteType::OVERWRITE),
              Os::File::OP_OK);
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OTHER_ERROR);
    ASSERT_EQ(file.flush(), Os::File::Status::OTHER_ERROR);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.lastCalled, Os::Stub::File::Test::StaticData::FLUSH_FN);
}

// Ensure that Os::File properly routes flush calls to the `flushInternal` function.
TEST_F(Interface, Read) {
    U8 buffer[] = {0xab, 0xcd, 0xef};
    FwSizeType size = static_cast<FwSizeType>(sizeof buffer);
    FwSizeType original_size = size;
    Os::File file;
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OP_OK);
    ASSERT_EQ(file.open("/does/not/matter", Os::File::OPEN_READ, Os::File::OverwriteType::OVERWRITE), Os::File::OP_OK);
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OTHER_ERROR);
    ASSERT_EQ(file.read(buffer, size, Os::File::WaitType::WAIT), Os::File::Status::OTHER_ERROR);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.lastCalled, Os::Stub::File::Test::StaticData::READ_FN);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.readBuffer, buffer);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.readSize, original_size);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.readWait, Os::File::WaitType::WAIT);
}

// Ensure that Os::File properly routes statuses returned from the `flushInternal` function back to the caller.
TEST_F(Interface, Write) {
    U8 buffer[] = {0xab, 0xcd, 0xef};
    FwSizeType size = static_cast<FwSizeType>(sizeof buffer);
    FwSizeType original_size = size;
    Os::File file;
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OP_OK);
    ASSERT_EQ(file.open("/does/not/matter", Os::File::OPEN_WRITE, Os::File::OverwriteType::OVERWRITE), Os::File::OP_OK);
    Os::Stub::File::Test::StaticData::setNextStatus(Os::File::OTHER_ERROR);
    ASSERT_EQ(file.write(buffer, size, Os::File::WaitType::WAIT), Os::File::Status::OTHER_ERROR);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.lastCalled, Os::Stub::File::Test::StaticData::WRITE_FN);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.writeBuffer, buffer);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.writeSize, original_size);
    ASSERT_EQ(Os::Stub::File::Test::StaticData::data.writeWait, Os::File::WaitType::WAIT);
}

int main(int argc, char** argv) {
    Os::init();
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
