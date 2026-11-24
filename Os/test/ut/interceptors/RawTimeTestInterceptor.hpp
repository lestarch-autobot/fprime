// ======================================================================
// \title Os/test/ut/interceptors/RawTimeTestInterceptor.hpp
// \brief TEST ONLY: interface used by unit tests to intercept Os::RawTime calls
//
// This interface is for unit testing only. It works with Os::Ut::RawTime::UtRawTime, the Os::RawTime implementation
// selected in unit test builds (BUILD_UT), and has no effect in any other build. Do not use it in flight code.
// ======================================================================
#ifndef OS_TEST_UT_INTERCEPTORS_RAWTIMETESTINTERCEPTOR_HPP_
#define OS_TEST_UT_INTERCEPTORS_RAWTIMETESTINTERCEPTOR_HPP_

#include <Os/Interfaces/RawTimeInterface.hpp>
#include <Os/test/ut/interceptors/TestInterceptor.hpp>

namespace Os {
namespace Test {

//! \brief TEST ONLY: intercepts the calls made on every Os::RawTime in a unit test build
//!
//! Each function mirrors the `RawTimeInterface` function of the same name with one extra first argument, `real`: the
//! implementation a non-test build would have used, belonging to the `Os::RawTime` on which the call was made.
//! Functions that are not overridden pass through to `real`; the lifetime notifications do nothing unless
//! overridden. See Os/test/ut/interceptors/TestInterceptor.hpp for installation and threading rules.
class RawTimeTestInterceptor : public RawTimeUserInterface, public TestInterceptor<RawTimeTestInterceptor> {
  public:
    RawTimeTestInterceptor() = default;
    virtual ~RawTimeTestInterceptor() = default;

    //! \brief notification: an Os::RawTime implementation was constructed
    virtual void constructed(RawTimeInterface& real);

    //! \brief notification: an Os::RawTime implementation is being destroyed
    virtual void destructed(RawTimeInterface& real);

    //! \brief notification: `real` was copy constructed from `other`
    virtual void copyConstructed(RawTimeInterface& real, const RawTimeInterface& other);

    //! \brief notification: `other` was copy assigned to `real`
    virtual void copyAssigned(RawTimeInterface& real, const RawTimeInterface& other);

    //! \brief intercept `RawTimeInterface::getHandle`
    virtual RawTimeHandle* getHandle(RawTimeInterface& real);

    //! \brief intercept `RawTimeInterface::now`
    virtual Status now(RawTimeInterface& real);

    //! \brief intercept `RawTimeInterface::getTimeInterval`
    virtual Status getTimeInterval(const RawTimeInterface& real,
                                   const RawTimeInterface& other,
                                   Fw::TimeInterval& interval);

    //! \brief intercept `RawTimeInterface::serializeTo`
    virtual Fw::SerializeStatus serializeTo(const RawTimeInterface& real,
                                            Fw::SerialBufferBase& buffer,
                                            Fw::Endianness mode);

    //! \brief intercept `RawTimeInterface::deserializeFrom`
    virtual Fw::SerializeStatus deserializeFrom(RawTimeInterface& real,
                                                Fw::SerialBufferBase& buffer,
                                                Fw::Endianness mode);
};

}  // namespace Test
}  // namespace Os

#endif  // OS_TEST_UT_INTERCEPTORS_RAWTIMETESTINTERCEPTOR_HPP_
