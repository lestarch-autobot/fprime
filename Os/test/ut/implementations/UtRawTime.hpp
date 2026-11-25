// ======================================================================
// \title Os/test/ut/implementations/UtRawTime.hpp
// \brief TEST ONLY: Os::RawTime implementation selected in unit test builds
// ======================================================================
#ifndef OS_TEST_UT_IMPLEMENTATIONS_UTRAWTIME_HPP_
#define OS_TEST_UT_IMPLEMENTATIONS_UTRAWTIME_HPP_

#include <Os/Interfaces/RawTimeInterface.hpp>
#include "config/RawTimeSelection.hpp"

namespace Os {
namespace Ut {
namespace RawTime {

//! \brief TEST ONLY: wraps the selected Os::RawTime implementation so that unit tests may intercept its calls
//!
//! Each call is forwarded to the installed `Os::Test::RawTimeTestInterceptor`, or directly to the wrapped
//! implementation (`Os::Selected::RawTime`) when no interceptor is installed.
class UtRawTime : public RawTimeInterface {
  public:
    UtRawTime();
    explicit UtRawTime(RawTimeSource source);
    UtRawTime(const UtRawTime& other);
    UtRawTime& operator=(const UtRawTime& other);
    ~UtRawTime() override;

    RawTimeHandle* getHandle() override;
    Status now() override;
    Status getTimeInterval(const RawTimeInterface& other, Fw::TimeInterval& interval) const override;
    Fw::SerializeStatus serializeTo(Fw::SerialBufferBase& buffer,
                                    Fw::Endianness mode = Fw::Endianness::BIG) const override;
    Fw::SerializeStatus deserializeFrom(Fw::SerialBufferBase& buffer,
                                        Fw::Endianness mode = Fw::Endianness::BIG) override;

  private:
    Os::Selected::RawTime m_real;  //!< Implementation selected for non-test builds
};

}  // namespace RawTime
}  // namespace Ut
}  // namespace Os

#endif  // OS_TEST_UT_IMPLEMENTATIONS_UTRAWTIME_HPP_
