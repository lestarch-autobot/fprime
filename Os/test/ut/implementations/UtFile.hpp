// ======================================================================
// \title Os/test/ut/implementations/UtFile.hpp
// \brief TEST ONLY: Os::File implementation selected in unit test builds
// ======================================================================
#ifndef OS_TEST_UT_IMPLEMENTATIONS_UTFILE_HPP_
#define OS_TEST_UT_IMPLEMENTATIONS_UTFILE_HPP_

#include <Os/Interfaces/FileInterface.hpp>
#include "config/FileSelection.hpp"

namespace Os {
namespace Ut {
namespace File {

//! \brief TEST ONLY: wraps the selected Os::File implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::FileTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::File`) when no interceptor is installed.
class UtFile : public FileInterface {
  public:
    UtFile();
    UtFile(const UtFile& other);
    UtFile& operator=(const UtFile& other);
    ~UtFile() override;

    Status open(const char* path, Mode mode, OverwriteType overwrite) override;
    void close() override;
    Status size(FwSizeType& size_result) override;
    Status position(FwSizeType& position_result) override;
    Status preallocate(FwSizeType offset, FwSizeType length) override;
    Status seek(FwSignedSizeType offset, SeekType seekType) override;
    Status flush() override;
    Status read(U8* buffer, FwSizeType& size, WaitType wait) override;
    Status write(const U8* buffer, FwSizeType& size, WaitType wait) override;
    FileHandle* getHandle() override;

  private:
    Os::Selected::File m_real;  //!< Implementation selected for non-test builds
};

}  // namespace File
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_IMPLEMENTATIONS_UTFILE_HPP_
