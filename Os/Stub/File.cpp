// ======================================================================
// \title Os/Stub/File.cpp
// \brief stub implementation for Os::File
// ======================================================================
#include "Os/Stub/File.hpp"
#include <Fw/Types/Assert.hpp>

namespace Os {
namespace Stub {
namespace File {

StubFile::Status StubFile::open(const char* filepath, StubFile::Mode open_mode, OverwriteType overwrite) {
    FW_ASSERT(nullptr != filepath);
    FW_ASSERT(Mode::OPEN_NO_MODE < open_mode && Mode::MAX_OPEN_MODE > open_mode);
    FW_ASSERT((0 <= overwrite) && (overwrite < OverwriteType::MAX_OVERWRITE_TYPE));
    // Check for already opened file
    if (this->isOpen()) {
        return Status::INVALID_MODE;
    }
    Status status = Status::NOT_SUPPORTED;
    return status;
}

void StubFile::close() {
    this->setMode(Mode::OPEN_NO_MODE);
}

StubFile::Status StubFile::size(FwSizeType& size_result) {
    // Check that the file is open before attempting operation
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    }
    Status status = Status::NOT_SUPPORTED;
    return status;
}

StubFile::Status StubFile::position(FwSizeType& position_result) {
    // Check that the file is open before attempting operation
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    }
    Status status = Status::NOT_SUPPORTED;
    return status;
}

StubFile::Status StubFile::preallocate(FwSizeType offset, FwSizeType length) {
    // Check that the file is open before attempting operation
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    } else if (this->getMode() == Mode::OPEN_READ) {
        return Status::INVALID_MODE;
    }
    Status status = Status::NOT_SUPPORTED;
    return status;
}

StubFile::Status StubFile::seek(FwSignedSizeType offset, SeekType seekType) {
    FW_ASSERT((0 <= seekType) && (seekType < SeekType::MAX_SEEK_TYPE));
    // Cannot do a seek with a negative offset in absolute mode
    FW_ASSERT((seekType == SeekType::RELATIVE) || (offset >= 0));
    // Check that the file is open before attempting operation
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    }
    Status status = Status::NOT_SUPPORTED;
    return status;
}

StubFile::Status StubFile::flush() {
    // Check that the file is open before attempting operation
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    } else if (this->getMode() == Mode::OPEN_READ) {
        return Status::INVALID_MODE;
    }
    Status status = Status::NOT_SUPPORTED;
    return status;
}

StubFile::Status StubFile::read(U8* buffer, FwSizeType& size, StubFile::WaitType wait) {
    FW_ASSERT(buffer != nullptr);
    // Check that the file is open before attempting operation
    if (not this->isOpen()) {
        size = 0;
        return Status::NOT_OPENED;
    } else if (this->getMode() != Mode::OPEN_READ) {
        size = 0;
        return Status::INVALID_MODE;
    }
    Status status = Status::NOT_SUPPORTED;
    return status;
}

StubFile::Status StubFile::write(const U8* buffer, FwSizeType& size, StubFile::WaitType wait) {
    FW_ASSERT(buffer != nullptr);
    // Check that the file is open before attempting operation
    if (not this->isOpen()) {
        size = 0;
        return Status::NOT_OPENED;
    } else if (this->getMode() == Mode::OPEN_READ) {
        size = 0;
        return Status::INVALID_MODE;
    }
    Status status = Status::NOT_SUPPORTED;
    return status;
}

FileHandle* StubFile::getHandle() {
    return &this->m_handle;
}

}  // namespace File
}  // namespace Stub
}  // namespace Os
