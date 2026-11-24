// ======================================================================
// \title Os/test/ut/implementations/UtFileSystem.hpp
// \brief TEST ONLY: Os::FileSystem implementation selected in unit test builds
// ======================================================================
// Included before the guard: Os/FileSystem.hpp includes this file, and may be reached from the interface header
#include <Os/Interfaces/FileSystemInterface.hpp>
#ifndef OS_TEST_UT_IMPLEMENTATIONS_UTFILESYSTEM_HPP_
#define OS_TEST_UT_IMPLEMENTATIONS_UTFILESYSTEM_HPP_

#ifndef OS_SELECTED_FILE_SYSTEM_HEADER
#error "OS_SELECTED_FILE_SYSTEM_HEADER must be defined in config/OsSelection.hpp"
#endif
#include OS_SELECTED_FILE_SYSTEM_HEADER

namespace Os {
namespace Ut {
namespace FileSystem {

//! \brief TEST ONLY: wraps the selected Os::FileSystem implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::FileSystemTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::FileSystem`) when no interceptor is installed.
class UtFileSystem : public FileSystemInterface {
  public:
    UtFileSystem();
    ~UtFileSystem() override;

    FileSystemHandle* getHandle() override;
    Status _removeDirectory(const char* path) override;
    Status _removeFile(const char* path) override;
    Status _rename(const char* sourcePath, const char* destPath) override;
    Status _getFreeSpace(const char* path, FwSizeType& totalBytes, FwSizeType& freeBytes) override;
    Status _getPathType(const char* path, PathType& pathType) override;
    Status _getWorkingDirectory(char* path, FwSizeType bufferSize) override;
    Status _changeWorkingDirectory(const char* path) override;

  private:
    Os::Selected::FileSystem m_real;  //!< Implementation selected for non-test builds
};

}  // namespace FileSystem
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_IMPLEMENTATIONS_UTFILESYSTEM_HPP_
