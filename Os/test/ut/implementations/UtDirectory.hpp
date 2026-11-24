// ======================================================================
// \title Os/test/ut/implementations/UtDirectory.hpp
// \brief TEST ONLY: Os::Directory implementation selected in unit test builds
// ======================================================================
// Included before the guard: Os/Directory.hpp includes this file, and may be reached from the interface header
#include <Os/Interfaces/DirectoryInterface.hpp>
#ifndef OS_TEST_UT_IMPLEMENTATIONS_UTDIRECTORY_HPP_
#define OS_TEST_UT_IMPLEMENTATIONS_UTDIRECTORY_HPP_

#ifndef OS_SELECTED_DIRECTORY_HEADER
#error "OS_SELECTED_DIRECTORY_HEADER must be defined in config/OsSelection.hpp"
#endif
#include OS_SELECTED_DIRECTORY_HEADER

namespace Os {
namespace Ut {
namespace Directory {

//! \brief TEST ONLY: wraps the selected Os::Directory implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::DirectoryTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::Directory`) when no interceptor is installed.
class UtDirectory : public DirectoryInterface {
  public:
    UtDirectory();
    ~UtDirectory() override;

    DirectoryHandle* getHandle() override;
    Status open(const char* path, OpenMode mode) override;
    Status rewind() override;
    Status read(char* fileNameBuffer, FwSizeType buffSize) override;
    void close() override;

  private:
    Os::Selected::Directory m_real;  //!< Implementation selected for non-test builds
};

}  // namespace Directory
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_IMPLEMENTATIONS_UTDIRECTORY_HPP_
