// ======================================================================
// \title Os/Interfaces/Test/RawTimeTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::RawTime calls
//
// This interface is for unit testing only. It works with Os::Ut::RawTime::UtRawTime, the Os::RawTime implementation
// selected in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight code.
// ======================================================================
#ifndef OS_INTERFACES_TEST_RAWTIMETESTINTERCEPTOR_HPP_
#define OS_INTERFACES_TEST_RAWTIMETESTINTERCEPTOR_HPP_

#include <Os/Interfaces/RawTimeInterface.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made by every Os::RawTime in a unit test build
//!
//! While an object of this class exists, each Os::RawTime call is routed to the matching function here along with
//! `real`, the implementation selected for non-test builds that backs the calling Os::RawTime. The default functions
//! forward to `real`. A test derives from this class and overrides only the functions it intercepts.
//!
//! \warning at most one interceptor may exist at a time. Construct and destroy it while no other thread is using
//! Os::RawTime: installation is not synchronized.
class RawTimeTestInterceptor : public RawTimeUserInterface {
  public:
    //! \brief install this interceptor: asserts that none is installed
    RawTimeTestInterceptor();

    //! \brief uninstall this interceptor
    virtual ~RawTimeTestInterceptor();

    RawTimeTestInterceptor(const RawTimeTestInterceptor& other) = delete;
    RawTimeTestInterceptor& operator=(const RawTimeTestInterceptor& other) = delete;

    //! \brief installed interceptor, or nullptr when none
    static RawTimeTestInterceptor* getInstalled();

    //! \brief intercept RawTimeInterface::now
    virtual Status now(RawTimeInterface& real);

    //! \brief intercept RawTimeInterface::getTimeInterval
    //! \param real: real implementation of the calling Os::RawTime
    //! \param other: real implementation of the Os::RawTime compared against
    virtual Status getTimeInterval(const RawTimeInterface& real,
                                   const RawTimeInterface& other,
                                   Fw::TimeInterval& interval);

    //! \brief intercept RawTimeInterface::serializeTo
    virtual Fw::SerializeStatus serializeTo(const RawTimeInterface& real,
                                            Fw::SerialBufferBase& buffer,
                                            Fw::Endianness mode);

    //! \brief intercept RawTimeInterface::deserializeFrom
    virtual Fw::SerializeStatus deserializeFrom(RawTimeInterface& real,
                                                Fw::SerialBufferBase& buffer,
                                                Fw::Endianness mode);

  private:
    static RawTimeTestInterceptor* s_installed;  //!< Installed interceptor, nullptr when none
};

}  // namespace Test
}  // namespace Os

#endif  // OS_INTERFACES_TEST_RAWTIMETESTINTERCEPTOR_HPP_
