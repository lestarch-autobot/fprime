#include <string.h>
#include <Os/Interfaces/FileInterface.hpp>
#include <Os/test/ut/interceptors/FileTestInterceptor.hpp>

#ifndef OS_STUB_FILE_TEST_HPP
#define OS_STUB_FILE_TEST_HPP
namespace Os {
namespace Stub {
namespace File {
namespace Test {

//! Data that supports the stubbed File implementation.
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

}  // namespace Test
}  // namespace File
}  // namespace Stub
}  // namespace Os
#endif  // OS_STUB_FILE_TEST_HPP
