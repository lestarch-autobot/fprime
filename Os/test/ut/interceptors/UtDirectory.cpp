// ======================================================================
// \title Os/test/ut/interceptors/UtDirectory.cpp
// \brief TEST ONLY: UtDirectory and DirectoryTestInterceptor default implementations
// ======================================================================
#include <Os/test/ut/interceptors/DirectoryTestInterceptor.hpp>
#include <Os/test/ut/interceptors/UtDirectory.hpp>

namespace Os {
namespace Test {

void DirectoryTestInterceptor::constructed(DirectoryInterface& /*real*/) {}

void DirectoryTestInterceptor::destructed(DirectoryInterface& /*real*/) {}

DirectoryHandle* DirectoryTestInterceptor::getHandle(DirectoryInterface& real) {
    return real.getHandle();
}

DirectoryTestInterceptor::Status DirectoryTestInterceptor::open(DirectoryInterface& real,
                                                                const char* path,
                                                                OpenMode mode) {
    return real.open(path, mode);
}

DirectoryTestInterceptor::Status DirectoryTestInterceptor::rewind(DirectoryInterface& real) {
    return real.rewind();
}

DirectoryTestInterceptor::Status DirectoryTestInterceptor::read(DirectoryInterface& real,
                                                                char* fileNameBuffer,
                                                                FwSizeType buffSize) {
    return real.read(fileNameBuffer, buffSize);
}

void DirectoryTestInterceptor::close(DirectoryInterface& real) {
    real.close();
}

}  // namespace Test

namespace Ut {
namespace Directory {

using Os::Test::DirectoryTestInterceptor;

UtDirectory::UtDirectory() {
    DirectoryTestInterceptor* interceptor = DirectoryTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real);
    }
}

UtDirectory::~UtDirectory() {
    DirectoryTestInterceptor* interceptor = DirectoryTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->destructed(this->m_real);
    }
}

DirectoryHandle* UtDirectory::getHandle() {
    DirectoryTestInterceptor* interceptor = DirectoryTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getHandle(this->m_real) : this->m_real.getHandle();
}

UtDirectory::Status UtDirectory::open(const char* path, OpenMode mode) {
    DirectoryTestInterceptor* interceptor = DirectoryTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->open(this->m_real, path, mode) : this->m_real.open(path, mode);
}

UtDirectory::Status UtDirectory::rewind() {
    DirectoryTestInterceptor* interceptor = DirectoryTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->rewind(this->m_real) : this->m_real.rewind();
}

UtDirectory::Status UtDirectory::read(char* fileNameBuffer, FwSizeType buffSize) {
    DirectoryTestInterceptor* interceptor = DirectoryTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->read(this->m_real, fileNameBuffer, buffSize)
                                    : this->m_real.read(fileNameBuffer, buffSize);
}

void UtDirectory::close() {
    DirectoryTestInterceptor* interceptor = DirectoryTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->close(this->m_real);
    } else {
        this->m_real.close();
    }
}

}  // namespace Directory
}  // namespace Ut
}  // namespace Os
