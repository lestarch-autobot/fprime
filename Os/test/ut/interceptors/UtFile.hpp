// ======================================================================
// \title Os/test/ut/interceptors/UtFile.hpp
// \brief TEST ONLY: Os::File implementation selected in unit test builds
// ======================================================================
#include <Os/Interfaces/FileInterface.hpp>
#ifndef OS_TEST_UT_INTERCEPTORS_UTFILE_HPP_
#define OS_TEST_UT_INTERCEPTORS_UTFILE_HPP_

// Validate that OS_UT_REAL_FILE_HEADER was defined by config/OsSelection.hpp
#ifndef OS_UT_REAL_FILE_HEADER
#error "OS_UT_REAL_FILE_HEADER must be defined in config/OsSelection.hpp"
#endif
#include OS_UT_REAL_FILE_HEADER

namespace Os {
namespace Ut {
namespace File {

//! \brief TEST ONLY: implementation of Os::File that lets a unit test intercept file calls
//!
//! Holds the implementation a non-test build would have selected (`RealFile`, set in config/OsSelection.hpp). Each call
//! goes to the installed `Os::Test::FileTestInterceptor`, which is handed that real implementation, or straight to
//! the real implementation when no interceptor is installed. `getHandle` is never intercepted.
class UtFile : public FileInterface {
  public:
    UtFile() = default;
    UtFile(const UtFile& other) = default;
    UtFile& operator=(const UtFile& other) = default;
    ~UtFile() override = default;

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
    RealFile m_real;  //!< Implementation selected for non-test builds
};

}  // namespace File
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_UTFILE_HPP_
