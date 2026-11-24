// ======================================================================
// \title Os/test/ut/interceptors/UtRawTime.cpp
// \brief TEST ONLY: Os::RawTime implementation selected in unit test builds
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/test/ut/interceptors/RawTimeTestInterceptor.hpp>
#include <Os/test/ut/interceptors/UtRawTime.hpp>

namespace Os {
namespace Test {

RawTimeTestInterceptor* RawTimeTestInterceptor::s_installed = nullptr;

RawTimeTestInterceptor::RawTimeTestInterceptor() {
    FW_ASSERT(s_installed == nullptr);
    s_installed = this;
}

RawTimeTestInterceptor::~RawTimeTestInterceptor() {
    s_installed = nullptr;
}

RawTimeTestInterceptor* RawTimeTestInterceptor::getInstalled() {
    return s_installed;
}

RawTimeTestInterceptor::Status RawTimeTestInterceptor::now(RawTimeInterface& real) {
    return real.now();
}

RawTimeTestInterceptor::Status RawTimeTestInterceptor::getTimeInterval(const RawTimeInterface& real,
                                                                       const RawTimeInterface& other,
                                                                       Fw::TimeInterval& interval) {
    return real.getTimeInterval(other, interval);
}

Fw::SerializeStatus RawTimeTestInterceptor::serializeTo(const RawTimeInterface& real,
                                                        Fw::SerialBufferBase& buffer,
                                                        Fw::Endianness mode) {
    return real.serializeTo(buffer, mode);
}

Fw::SerializeStatus RawTimeTestInterceptor::deserializeFrom(RawTimeInterface& real,
                                                            Fw::SerialBufferBase& buffer,
                                                            Fw::Endianness mode) {
    return real.deserializeFrom(buffer, mode);
}

}  // namespace Test

namespace Ut {
namespace RawTime {

UtRawTime::UtRawTime(RawTimeSource source) : m_real(source) {}

RawTimeHandle* UtRawTime::getHandle() {
    return this->m_real.getHandle();
}

UtRawTime::Status UtRawTime::now() {
    Os::Test::RawTimeTestInterceptor* interceptor = Os::Test::RawTimeTestInterceptor::getInstalled();
    return (interceptor == nullptr) ? this->m_real.now() : interceptor->now(this->m_real);
}

UtRawTime::Status UtRawTime::getTimeInterval(const RawTimeInterface& other, Fw::TimeInterval& interval) const {
    // Os::RawTimeCommon only compares raw times of its own implementation type
    const RawTimeInterface& other_real = static_cast<const UtRawTime&>(other).m_real;
    Os::Test::RawTimeTestInterceptor* interceptor = Os::Test::RawTimeTestInterceptor::getInstalled();
    return (interceptor == nullptr) ? this->m_real.getTimeInterval(other_real, interval)
                                    : interceptor->getTimeInterval(this->m_real, other_real, interval);
}

Fw::SerializeStatus UtRawTime::serializeTo(Fw::SerialBufferBase& buffer, Fw::Endianness mode) const {
    Os::Test::RawTimeTestInterceptor* interceptor = Os::Test::RawTimeTestInterceptor::getInstalled();
    return (interceptor == nullptr) ? this->m_real.serializeTo(buffer, mode)
                                    : interceptor->serializeTo(this->m_real, buffer, mode);
}

Fw::SerializeStatus UtRawTime::deserializeFrom(Fw::SerialBufferBase& buffer, Fw::Endianness mode) {
    Os::Test::RawTimeTestInterceptor* interceptor = Os::Test::RawTimeTestInterceptor::getInstalled();
    return (interceptor == nullptr) ? this->m_real.deserializeFrom(buffer, mode)
                                    : interceptor->deserializeFrom(this->m_real, buffer, mode);
}

}  // namespace RawTime
}  // namespace Ut
}  // namespace Os
