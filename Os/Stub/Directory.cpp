// ======================================================================
// \title Os/Stub/Directory.cpp
// \brief stub implementation for Os::Directory
// ======================================================================
#include "Os/Stub/Directory.hpp"
#include <Fw/Types/Assert.hpp>

namespace Os {
namespace Stub {
namespace Directory {

StubDirectory::Status StubDirectory::open(const char* path, OpenMode mode) {
    FW_ASSERT(path != nullptr);
    FW_ASSERT(mode >= 0 and mode < OpenMode::MAX_OPEN_MODE);
    return Status::NOT_SUPPORTED;
}

StubDirectory::Status StubDirectory::rewind() {
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    }
    return Status::NOT_SUPPORTED;
}

StubDirectory::Status StubDirectory::read(char* fileNameBuffer, FwSizeType bufSize) {
    if (not this->isOpen()) {
        return Status::NOT_OPENED;
    }
    // Unreachable while open() cannot succeed; kept for parity with the other implementations
    FW_ASSERT(fileNameBuffer != nullptr);
    FW_ASSERT(bufSize > 0);
    fileNameBuffer[bufSize - 1] = '\0';
    return Status::NOT_SUPPORTED;
}

void StubDirectory::close() {
    // no-op
}

bool StubDirectory::isOpenImpl() const {
    return false;
}

DirectoryHandle* StubDirectory::getHandle() {
    return &this->m_handle;
}

}  // namespace Directory
}  // namespace Stub
}  // namespace Os
