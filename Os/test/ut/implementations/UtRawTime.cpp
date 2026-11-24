// ======================================================================
// \title Os/test/ut/implementations/UtRawTime.cpp
// \brief TEST ONLY: UtRawTime and RawTimeTestInterceptor default implementations
// ======================================================================
#include <Os/test/ut/implementations/UtRawTime.hpp>
#include <Os/test/ut/interceptors/RawTimeTestInterceptor.hpp>

namespace Os {
namespace Test {

void RawTimeTestInterceptor::constructed(RawTimeInterface& /*real*/) {}

void RawTimeTestInterceptor::destructed(RawTimeInterface& /*real*/) {}

void RawTimeTestInterceptor::copyConstructed(RawTimeInterface& /*real*/, const RawTimeInterface& /*other*/) {}

void RawTimeTestInterceptor::copyAssigned(RawTimeInterface& /*real*/, const RawTimeInterface& /*other*/) {}

RawTimeHandle* RawTimeTestInterceptor::getHandle(RawTimeInterface& real) {
    return real.getHandle();
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

using Os::Test::RawTimeTestInterceptor;

UtRawTime::UtRawTime() {
    RawTimeTestInterceptor* interceptor = RawTimeTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real);
    }
}

UtRawTime::UtRawTime(RawTimeSource source) : m_real(source) {
    RawTimeTestInterceptor* interceptor = RawTimeTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->constructed(this->m_real);
    }
}

UtRawTime::UtRawTime(const UtRawTime& other) : m_real(other.m_real) {
    RawTimeTestInterceptor* interceptor = RawTimeTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->copyConstructed(this->m_real, other.m_real);
    }
}

UtRawTime& UtRawTime::operator=(const UtRawTime& other) {
    if (this != &other) {
        this->m_real = other.m_real;
    }
    RawTimeTestInterceptor* interceptor = RawTimeTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->copyAssigned(this->m_real, other.m_real);
    }
    return *this;
}

UtRawTime::~UtRawTime() {
    RawTimeTestInterceptor* interceptor = RawTimeTestInterceptor::getInstalled();
    if (interceptor != nullptr) {
        interceptor->destructed(this->m_real);
    }
}

RawTimeHandle* UtRawTime::getHandle() {
    RawTimeTestInterceptor* interceptor = RawTimeTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getHandle(this->m_real) : this->m_real.getHandle();
}

UtRawTime::Status UtRawTime::now() {
    RawTimeTestInterceptor* interceptor = RawTimeTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->now(this->m_real) : this->m_real.now();
}

UtRawTime::Status UtRawTime::getTimeInterval(const RawTimeInterface& other, Fw::TimeInterval& interval) const {
    // Os::RawTimeCommon<UtRawTime> only supplies another UtRawTime: hand the wrapped implementation to the real one
    const RawTimeInterface& other_real = static_cast<const UtRawTime&>(other).m_real;
    RawTimeTestInterceptor* interceptor = RawTimeTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->getTimeInterval(this->m_real, other_real, interval)
                                    : this->m_real.getTimeInterval(other_real, interval);
}

Fw::SerializeStatus UtRawTime::serializeTo(Fw::SerialBufferBase& buffer, Fw::Endianness mode) const {
    RawTimeTestInterceptor* interceptor = RawTimeTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->serializeTo(this->m_real, buffer, mode)
                                    : this->m_real.serializeTo(buffer, mode);
}

Fw::SerializeStatus UtRawTime::deserializeFrom(Fw::SerialBufferBase& buffer, Fw::Endianness mode) {
    RawTimeTestInterceptor* interceptor = RawTimeTestInterceptor::getInstalled();
    return (interceptor != nullptr) ? interceptor->deserializeFrom(this->m_real, buffer, mode)
                                    : this->m_real.deserializeFrom(buffer, mode);
}

}  // namespace RawTime
}  // namespace Ut
}  // namespace Os
