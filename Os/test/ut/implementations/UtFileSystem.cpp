// ======================================================================
// \title Os/test/ut/implementations/UtFileSystem.cpp
// \brief TEST ONLY: UtFileSystem and FileSystemTestInterceptor default implementations
// ======================================================================
#include <Os/test/ut/implementations/UtFileSystem.hpp>
#include <Os/test/ut/interceptors/FileSystemTestInterceptor.hpp>

namespace Os {
namespace Test {

void FileSystemTestInterceptor::constructed(FileSystemInterface& /*real*/) {}

void FileSystemTestInterceptor::destructed(FileSystemInterface& /*real*/) {}

FileSystemHandle* FileSystemTestInterceptor::getHandle(FileSystemInterface& real) {
    return real.getHandle();
}

FileSystemTestInterceptor::Status FileSystemTestInterceptor::_removeDirectory(FileSystemInterface& real,
                                                                              const char* path) {
    return real._removeDirectory(path);
}

FileSystemTestInterceptor::Status FileSystemTestInterceptor::_removeFile(FileSystemInterface& real, const char* path) {
    return real._removeFile(path);
}

FileSystemTestInterceptor::Status FileSystemTestInterceptor::_rename(FileSystemInterface& real,
                                                                     const char* sourcePath,
                                                                     const char* destPath) {
    return real._rename(sourcePath, destPath);
}

FileSystemTestInterceptor::Status FileSystemTestInterceptor::_getFreeSpace(FileSystemInterface& real,
                                                                           const char* path,
                                                                           FwSizeType& totalBytes,
                                                                           FwSizeType& freeBytes) {
    return real._getFreeSpace(path, totalBytes, freeBytes);
}

FileSystemTestInterceptor::Status FileSystemTestInterceptor::_getPathType(FileSystemInterface& real,
                                                                          const char* path,
                                                                          PathType& pathType) {
    return real._getPathType(path, pathType);
}

FileSystemTestInterceptor::Status FileSystemTestInterceptor::_getWorkingDirectory(FileSystemInterface& real,
                                                                                  char* path,
                                                                                  FwSizeType bufferSize) {
    return real._getWorkingDirectory(path, bufferSize);
}

FileSystemTestInterceptor::Status FileSystemTestInterceptor::_changeWorkingDirectory(FileSystemInterface& real,
                                                                                     const char* path) {
    return real._changeWorkingDirectory(path);
}

}  // namespace Test

namespace Ut {
namespace FileSystem {

using Os::Test::FileSystemTestInterceptor;

UtFileSystem::UtFileSystem() {
    FileSystemTestInterceptor* interceptor = FileSystemTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real);
    }
}

UtFileSystem::~UtFileSystem() {
    FileSystemTestInterceptor* interceptor = FileSystemTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->destructed(this->m_real);
    }
}

FileSystemHandle* UtFileSystem::getHandle() {
    FileSystemTestInterceptor* interceptor = FileSystemTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getHandle(this->m_real) : this->m_real.getHandle();
}

UtFileSystem::Status UtFileSystem::_removeDirectory(const char* path) {
    FileSystemTestInterceptor* interceptor = FileSystemTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->_removeDirectory(this->m_real, path)
                                    : this->m_real._removeDirectory(path);
}

UtFileSystem::Status UtFileSystem::_removeFile(const char* path) {
    FileSystemTestInterceptor* interceptor = FileSystemTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->_removeFile(this->m_real, path) : this->m_real._removeFile(path);
}

UtFileSystem::Status UtFileSystem::_rename(const char* sourcePath, const char* destPath) {
    FileSystemTestInterceptor* interceptor = FileSystemTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->_rename(this->m_real, sourcePath, destPath)
                                    : this->m_real._rename(sourcePath, destPath);
}

UtFileSystem::Status UtFileSystem::_getFreeSpace(const char* path, FwSizeType& totalBytes, FwSizeType& freeBytes) {
    FileSystemTestInterceptor* interceptor = FileSystemTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->_getFreeSpace(this->m_real, path, totalBytes, freeBytes)
                                    : this->m_real._getFreeSpace(path, totalBytes, freeBytes);
}

UtFileSystem::Status UtFileSystem::_getPathType(const char* path, PathType& pathType) {
    FileSystemTestInterceptor* interceptor = FileSystemTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->_getPathType(this->m_real, path, pathType)
                                    : this->m_real._getPathType(path, pathType);
}

UtFileSystem::Status UtFileSystem::_getWorkingDirectory(char* path, FwSizeType bufferSize) {
    FileSystemTestInterceptor* interceptor = FileSystemTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->_getWorkingDirectory(this->m_real, path, bufferSize)
                                    : this->m_real._getWorkingDirectory(path, bufferSize);
}

UtFileSystem::Status UtFileSystem::_changeWorkingDirectory(const char* path) {
    FileSystemTestInterceptor* interceptor = FileSystemTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->_changeWorkingDirectory(this->m_real, path)
                                    : this->m_real._changeWorkingDirectory(path);
}

}  // namespace FileSystem
}  // namespace Ut
}  // namespace Os
