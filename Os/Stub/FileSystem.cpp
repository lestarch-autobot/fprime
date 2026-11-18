// ======================================================================
// \title Os/Stub/File.cpp
// \brief stub implementation for Os::File
// ======================================================================
#include "Os/Stub/FileSystem.hpp"
#include "Fw/Types/Assert.hpp"

namespace Os {
namespace Stub {
namespace FileSystem {

StubFileSystem::Status StubFileSystem::_removeDirectory(const char* path) {
    FW_ASSERT(path != nullptr);
    return Status::NOT_SUPPORTED;
}

StubFileSystem::Status StubFileSystem::_removeFile(const char* path) {
    FW_ASSERT(path != nullptr);
    return Status::NOT_SUPPORTED;
}

StubFileSystem::Status StubFileSystem::_rename(const char* originPath, const char* destPath) {
    FW_ASSERT(originPath != nullptr);
    FW_ASSERT(destPath != nullptr);
    return Status::NOT_SUPPORTED;
}

StubFileSystem::Status StubFileSystem::_getWorkingDirectory(char* path, FwSizeType bufferSize) {
    FW_ASSERT(path != nullptr);
    FW_ASSERT(bufferSize > 0);
    return Status::NOT_SUPPORTED;
}

StubFileSystem::Status StubFileSystem::_changeWorkingDirectory(const char* path) {
    FW_ASSERT(path != nullptr);
    return Status::NOT_SUPPORTED;
}

StubFileSystem::Status StubFileSystem::_getFreeSpace(const char* path, FwSizeType& totalBytes, FwSizeType& freeBytes) {
    FW_ASSERT(path != nullptr);
    return Status::NOT_SUPPORTED;
}

FileSystemHandle* StubFileSystem::getHandle() {
    return &this->m_handle;
}

StubFileSystem::Status StubFileSystem::_getPathType(const char* path, PathType& pathType) {
    FW_ASSERT(path != nullptr);
    return Status::NOT_SUPPORTED;
}

}  // namespace FileSystem
}  // namespace Stub
}  // namespace Os
