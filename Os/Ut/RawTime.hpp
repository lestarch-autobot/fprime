// ======================================================================
// \title Os/Ut/RawTime.hpp
// \brief TEST ONLY: Os::RawTime implementation selected in unit test builds
//
// Wraps the implementation selected for non-test builds and routes calls through the installed
// Os::Test::RawTimeTestInterceptor, when there is one. Do not use in flight code.
// ======================================================================
#include <Os/Interfaces/RawTimeInterface.hpp>

#ifndef OS_UT_RAWTIME_HPP_
#define OS_UT_RAWTIME_HPP_

#ifndef OS_UT_REAL_RAW_TIME_HEADER
#error "OS_UT_REAL_RAW_TIME_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_UT_REAL_RAW_TIME_HEADER

namespace Os {
namespace Ut {
namespace RawTime {

//! \brief TEST ONLY: Os::RawTime implementation holding the real implementation by value
class UtRawTime : public RawTimeInterface {
  public:
    UtRawTime() = default;
    explicit UtRawTime(RawTimeSource source);
    UtRawTime(const UtRawTime& other) = default;
    UtRawTime& operator=(const UtRawTime& other) = default;
    ~UtRawTime() override = default;

    RawTimeHandle* getHandle() override;
    Status now() override;
    Status getTimeInterval(const RawTimeInterface& other, Fw::TimeInterval& interval) const override;
    Fw::SerializeStatus serializeTo(Fw::SerialBufferBase& buffer,
                                    Fw::Endianness mode = Fw::Endianness::BIG) const override;
    Fw::SerializeStatus deserializeFrom(Fw::SerialBufferBase& buffer,
                                        Fw::Endianness mode = Fw::Endianness::BIG) override;

  private:
    RealRawTime m_real;  //!< Implementation selected for non-test builds
};

}  // namespace RawTime
}  // namespace Ut
}  // namespace Os

#endif  // OS_UT_RAWTIME_HPP_
