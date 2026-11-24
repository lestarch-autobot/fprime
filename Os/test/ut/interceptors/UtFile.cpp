// ======================================================================
// \title Os/test/ut/interceptors/UtFile.cpp
// \brief TEST ONLY: Os::File implementation selected in unit test builds
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/test/ut/interceptors/FileTestInterceptor.hpp>
#include <Os/test/ut/interceptors/UtFile.hpp>

namespace Os {
namespace Test {

FileTestInterceptor* FileTestInterceptor::s_installed = nullptr;

FileTestInterceptor::FileTestInterceptor() {
    FW_ASSERT(s_installed == nullptr);
    s_installed = this;
}

FileTestInterceptor::~FileTestInterceptor() {
    s_installed = nullptr;
}

FileTestInterceptor* FileTestInterceptor::getInstalled() {
    return s_installed;
}

FileTestInterceptor::Status FileTestInterceptor::open(FileInterface& real,
                                                      const char* path,
                                                      Mode mode,
                                                      OverwriteType overwrite) {
    return real.open(path, mode, overwrite);
}

void FileTestInterceptor::close(FileInterface& real) {
    real.close();
}

FileTestInterceptor::Status FileTestInterceptor::size(FileInterface& real, FwSizeType& size_result) {
    return real.size(size_result);
}

FileTestInterceptor::Status FileTestInterceptor::position(FileInterface& real, FwSizeType& position_result) {
    return real.position(position_result);
}

FileTestInterceptor::Status FileTestInterceptor::preallocate(FileInterface& real,
                                                             FwSizeType offset,
                                                             FwSizeType length) {
    return real.preallocate(offset, length);
}

FileTestInterceptor::Status FileTestInterceptor::seek(FileInterface& real, FwSignedSizeType offset, SeekType seekType) {
    return real.seek(offset, seekType);
}

FileTestInterceptor::Status FileTestInterceptor::flush(FileInterface& real) {
    return real.flush();
}

FileTestInterceptor::Status FileTestInterceptor::read(FileInterface& real,
                                                      U8* buffer,
                                                      FwSizeType& size,
                                                      WaitType wait) {
    return real.read(buffer, size, wait);
}

FileTestInterceptor::Status FileTestInterceptor::write(FileInterface& real,
                                                       const U8* buffer,
                                                       FwSizeType& size,
                                                       WaitType wait) {
    return real.write(buffer, size, wait);
}

}  // namespace Test

namespace Ut {
namespace File {

using Os::Test::FileTestInterceptor;

UtFile::Status UtFile::open(const char* path, Mode mode, OverwriteType overwrite) {
    FileTestInterceptor* interceptor = FileTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->open(this->m_real, path, mode, overwrite)
                                    : this->m_real.open(path, mode, overwrite);
}

void UtFile::close() {
    FileTestInterceptor* interceptor = FileTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->close(this->m_real);
    } else {
        this->m_real.close();
    }
}

UtFile::Status UtFile::size(FwSizeType& size_result) {
    FileTestInterceptor* interceptor = FileTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->size(this->m_real, size_result) : this->m_real.size(size_result);
}

UtFile::Status UtFile::position(FwSizeType& position_result) {
    FileTestInterceptor* interceptor = FileTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->position(this->m_real, position_result)
                                    : this->m_real.position(position_result);
}

UtFile::Status UtFile::preallocate(FwSizeType offset, FwSizeType length) {
    FileTestInterceptor* interceptor = FileTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->preallocate(this->m_real, offset, length)
                                    : this->m_real.preallocate(offset, length);
}

UtFile::Status UtFile::seek(FwSignedSizeType offset, SeekType seekType) {
    FileTestInterceptor* interceptor = FileTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->seek(this->m_real, offset, seekType)
                                    : this->m_real.seek(offset, seekType);
}

UtFile::Status UtFile::flush() {
    FileTestInterceptor* interceptor = FileTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->flush(this->m_real) : this->m_real.flush();
}

UtFile::Status UtFile::read(U8* buffer, FwSizeType& size, WaitType wait) {
    FileTestInterceptor* interceptor = FileTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->read(this->m_real, buffer, size, wait)
                                    : this->m_real.read(buffer, size, wait);
}

UtFile::Status UtFile::write(const U8* buffer, FwSizeType& size, WaitType wait) {
    FileTestInterceptor* interceptor = FileTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->write(this->m_real, buffer, size, wait)
                                    : this->m_real.write(buffer, size, wait);
}

FileHandle* UtFile::getHandle() {
    return this->m_real.getHandle();
}

}  // namespace File
}  // namespace Ut
}  // namespace Os
