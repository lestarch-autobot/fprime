// ======================================================================
// \title Os/test/ut/interceptors/DirectoryTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::Directory calls
//
// This interface is for unit testing only. It works with Os::Ut::Directory::UtDirectory, the Os::Directory
// implementation selected in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight
// code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_DIRECTORYTESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_DIRECTORYTESTINTERCEPTOR_HPP_

#include <Os/Interfaces/DirectoryInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::Directory in a unit test build
//!
//! Each function mirrors the `DirectoryInterface` function of the same name with one extra first argument, `real`: the
//! implementation a non-test build would have used, belonging to the `Os::Directory` on which the call was made.
//! Functions that are not overridden pass through to `real`; the lifetime notifications do nothing unless
//! overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class DirectoryTestInterceptor : public DirectoryUserInterface, public TestInterceptor<DirectoryTestInterceptor> {
  public:
    DirectoryTestInterceptor() = default;
    virtual ~DirectoryTestInterceptor() = default;

    //! \brief notification: an Os::Directory implementation was constructed
    virtual void constructed(DirectoryInterface& real);

    //! \brief notification: an Os::Directory implementation is being destroyed
    virtual void destructed(DirectoryInterface& real);

    //! \brief intercept `DirectoryInterface::getHandle`
    virtual DirectoryHandle* getHandle(DirectoryInterface& real);

    //! \brief intercept `DirectoryInterface::open`
    virtual Status open(DirectoryInterface& real, const char* path, OpenMode mode);

    //! \brief intercept `DirectoryInterface::rewind`
    virtual Status rewind(DirectoryInterface& real);

    //! \brief intercept `DirectoryInterface::read`
    virtual Status read(DirectoryInterface& real, char* fileNameBuffer, FwSizeType buffSize);

    //! \brief intercept `DirectoryInterface::close`
    virtual void close(DirectoryInterface& real);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_DIRECTORYTESTINTERCEPTOR_HPP_
