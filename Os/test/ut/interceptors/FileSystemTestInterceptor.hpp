// ======================================================================
// \title Os/test/ut/interceptors/FileSystemTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::FileSystem calls
//
// This interface is for unit testing only. It works with Os::Ut::FileSystem::UtFileSystem, the Os::FileSystem
// implementation selected in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight
// code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_FILESYSTEMTESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_FILESYSTEMTESTINTERCEPTOR_HPP_

#include <Os/Interfaces/FileSystemInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::FileSystem in a unit test build
//!
//! Each function mirrors the `FileSystemInterface` function of the same name with one extra first argument, `real`: the
//! implementation a non-test build would have used, belonging to the `Os::FileSystem` on which the call was made.
//! Functions that are not overridden pass through to `real`; the lifetime notifications do nothing unless
//! overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class FileSystemTestInterceptor : public FileSystemUserInterface, public TestInterceptor<FileSystemTestInterceptor> {
  public:
    FileSystemTestInterceptor() = default;
    virtual ~FileSystemTestInterceptor() = default;

    //! \brief notification: an Os::FileSystem implementation was constructed
    virtual void constructed(FileSystemInterface& real);

    //! \brief notification: an Os::FileSystem implementation is being destroyed
    virtual void destructed(FileSystemInterface& real);

    //! \brief intercept `FileSystemInterface::getHandle`
    virtual FileSystemHandle* getHandle(FileSystemInterface& real);

    //! \brief intercept `FileSystemInterface::_removeDirectory`
    virtual Status _removeDirectory(FileSystemInterface& real, const char* path);

    //! \brief intercept `FileSystemInterface::_removeFile`
    virtual Status _removeFile(FileSystemInterface& real, const char* path);

    //! \brief intercept `FileSystemInterface::_rename`
    virtual Status _rename(FileSystemInterface& real, const char* sourcePath, const char* destPath);

    //! \brief intercept `FileSystemInterface::_getFreeSpace`
    virtual Status _getFreeSpace(FileSystemInterface& real,
                                 const char* path,
                                 FwSizeType& totalBytes,
                                 FwSizeType& freeBytes);

    //! \brief intercept `FileSystemInterface::_getPathType`
    virtual Status _getPathType(FileSystemInterface& real, const char* path, PathType& pathType);

    //! \brief intercept `FileSystemInterface::_getWorkingDirectory`
    virtual Status _getWorkingDirectory(FileSystemInterface& real, char* path, FwSizeType bufferSize);

    //! \brief intercept `FileSystemInterface::_changeWorkingDirectory`
    virtual Status _changeWorkingDirectory(FileSystemInterface& real, const char* path);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_FILESYSTEMTESTINTERCEPTOR_HPP_
