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

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::File in a unit test build
//!
//! A unit test subclasses this interface and overrides the functions it wants to intercept. Each function mirrors the
//! `FileInterface` function of the same name with one extra first argument, `real`: the implementation a non-test
//! build would have used (e.g. `PosixFile`) belonging to the `Os::File` on which the call was made. An override may
//! call `real`, alter its results, or ignore it entirely. Functions that are not overridden pass through to `real`.
//!
//! The interceptor is active from construction to destruction, and at most one may exist at a time. While active it
//! receives the calls of every `Os::File` in the process, from any thread: construct it before starting tasks that use
//! files, destroy it after they stop, and guard any state it shares between threads.
//!
//! ```c++
//! class FailingOpen : public Os::Test::FileTestInterceptor {
//!     Status open(Os::FileInterface& real, const char* path, Mode mode, OverwriteType overwrite) override {
//!         return this->m_fail ? Os::File::NO_PERMISSION : real.open(path, mode, overwrite);
//!     }
//!     bool m_fail = false;
//! };
//! ```
class FileTestInterceptor : public FileUserInterface {
  public:
    //! \brief constructor: installs this interceptor, asserting that no other interceptor is installed
    FileTestInterceptor();

    //! \brief destructor: removes this interceptor
    virtual ~FileTestInterceptor();

    FileTestInterceptor(const FileTestInterceptor& other) = delete;
    FileTestInterceptor& operator=(const FileTestInterceptor& other) = delete;

    //! \brief get the installed interceptor
    //! \return installed interceptor, or nullptr when none is installed
    static FileTestInterceptor* getInstalled();

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

  private:
    static FileTestInterceptor* s_installed;  //!< Installed interceptor, nullptr when none
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_FILETESTINTERCEPTOR_HPP_
