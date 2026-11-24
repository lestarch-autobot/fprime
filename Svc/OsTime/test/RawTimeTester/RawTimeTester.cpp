// ======================================================================
// \title RawTimeTester
// \brief Test interceptor controlling the time read by Os::RawTime
// ======================================================================
#include "Svc/OsTime/test/RawTimeTester/RawTimeTester.hpp"
#include "Fw/Types/Assert.hpp"
#include "Fw/Types/SerialBuffer.hpp"

namespace Svc {

Fw::Time RawTimeTester::s_now_time;
RawTimeTester::Status RawTimeTester::s_now_status = RawTimeTester::OP_OK;

RawTimeTester::Status RawTimeTester::now(Os::RawTimeInterface& real) {
    if (s_now_status != OP_OK) {
        return s_now_status;
    }
    U8 buf_data[SERIALIZED_SIZE];
    Fw::SerialBuffer buf(buf_data, sizeof(buf_data));
    Fw::SerializeStatus status = buf.serializeFrom(s_now_time.getSeconds());
    FW_ASSERT(status == Fw::FW_SERIALIZE_OK, status);
    status = buf.serializeFrom(static_cast<U32>(s_now_time.getUSeconds() * 1000));
    FW_ASSERT(status == Fw::FW_SERIALIZE_OK, status);
    status = real.deserializeFrom(buf);
    FW_ASSERT(status == Fw::FW_SERIALIZE_OK, status);
    return OP_OK;
}

}  // namespace Svc
