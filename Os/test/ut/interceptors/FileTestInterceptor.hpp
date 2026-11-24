// ======================================================================
// \title Os/test/ut/interceptors/FileTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::File calls
//
// This interface is for unit testing only. It works with Os::Ut::File::UtFile, the Os::File implementation selected
// in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_FILETESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_FILETESTINTERCEPTOR_HPP_

#include <Os/Interfaces/FileInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::File in a unit test build
//!
//! Each function mirrors the `FileInterface` function of the same name with one extra first argument, `real`: the
//! implementation a non-test build would have used, belonging to the `Os::File` on which the call was made.
//! Functions that are not overridden pass through to `real`; the lifetime notifications do nothing unless
//! overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class FileTestInterceptor : public FileUserInterface, public TestInterceptor<FileTestInterceptor> {
  public:
    FileTestInterceptor() = default;
    virtual ~FileTestInterceptor() = default;

    //! \brief notification: an Os::File implementation was constructed
    virtual void constructed(FileInterface& real);

    //! \brief notification: an Os::File implementation is being destroyed
    virtual void destructed(FileInterface& real);

    //! \brief notification: `real` was copy constructed from `other`
    virtual void copyConstructed(FileInterface& real, const FileInterface& other);

    //! \brief notification: `other` was copy assigned to `real`
    virtual void copyAssigned(FileInterface& real, const FileInterface& other);

    //! \brief intercept `FileInterface::open`
    virtual Status open(FileInterface& real, const char* path, Mode mode, OverwriteType overwrite);

    //! \brief intercept `FileInterface::close`
    virtual void close(FileInterface& real);

    //! \brief intercept `FileInterface::size`
    virtual Status size(FileInterface& real, FwSizeType& size_result);

    //! \brief intercept `FileInterface::position`
    virtual Status position(FileInterface& real, FwSizeType& position_result);

    //! \brief intercept `FileInterface::preallocate`
    virtual Status preallocate(FileInterface& real, FwSizeType offset, FwSizeType length);

    //! \brief intercept `FileInterface::seek`
    virtual Status seek(FileInterface& real, FwSignedSizeType offset, SeekType seekType);

    //! \brief intercept `FileInterface::flush`
    virtual Status flush(FileInterface& real);

    //! \brief intercept `FileInterface::read`
    virtual Status read(FileInterface& real, U8* buffer, FwSizeType& size, WaitType wait);

    //! \brief intercept `FileInterface::write`
    virtual Status write(FileInterface& real, const U8* buffer, FwSizeType& size, WaitType wait);

    //! \brief intercept `FileInterface::getHandle`
    virtual FileHandle* getHandle(FileInterface& real);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_FILETESTINTERCEPTOR_HPP_
