// ======================================================================
// \title RawTimeTester
// \brief Test interceptor controlling the time read by Os::RawTime
// ======================================================================
#ifndef SVC_OSTIME_TEST_UT_RAWTIMETESTER_HPP
#define SVC_OSTIME_TEST_UT_RAWTIMETESTER_HPP

#include "Fw/Time/Time.hpp"
#include "Os/Interfaces/Test/RawTimeTestInterceptor.hpp"

namespace Svc {

//! \brief intercepts Os::RawTime::now to supply a test-controlled time
//!
//! While an object of this class exists, `now()` on any Os::RawTime sets the test-controlled time and returns the
//! test-controlled status. All other operations are performed by the real implementation.
class RawTimeTester : public Os::Test::RawTimeTestInterceptor {
  public:
    RawTimeTester() = default;

    ~RawTimeTester() override = default;

    //! \brief set the real raw time to the test-controlled time, or fail with the test-controlled status
    //!
    //! The time is loaded through deserializeFrom as a (seconds, nanoseconds) pair of U32, the serialized form of
    //! the Posix implementation selected in config/OsSelection.hpp.
    Status now(Os::RawTimeInterface& real) override;

    static void setNowTime(const Fw::Time&& t) { s_now_time = t; }

    static void setNowStatus(Status status) { s_now_status = status; }

  private:
    static Fw::Time s_now_time;
    static Status s_now_status;
};

}  // namespace Svc

#endif  // SVC_OSTIME_TEST_UT_RAWTIMETESTER_HPP
