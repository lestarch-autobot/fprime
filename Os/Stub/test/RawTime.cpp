// ======================================================================
// \title Os/Stub/RawTime.cpp
// \brief stub implementation for Os::RawTime
// ======================================================================
#include "Os/Stub/test/RawTime.hpp"

namespace Os {
namespace Stub {
namespace RawTime {
namespace Test {

StaticData StaticData::data;

TestRawTime::TestRawTime() {
    StaticData::data.lastCalled = StaticData::LastFn::CONSTRUCT_FN;
}

TestRawTime::TestRawTime(RawTimeSource source) : m_source(source) {
    StaticData::data.lastCalled = StaticData::LastFn::CONSTRUCT_FN;
    StaticData::data.lastSource = source;
}

TestRawTime::TestRawTime(const TestRawTime& other) : m_source(other.m_source) {
    StaticData::data.lastCalled = StaticData::LastFn::COPY_CONSTRUCT_FN;
}

TestRawTime::~TestRawTime() {
    StaticData::data.lastCalled = StaticData::LastFn::DESTRUCT_FN;
}

RawTimeSource TestRawTime::getSource() const {
    return this->m_source;
}

RawTimeHandle* TestRawTime::getHandle() {
    StaticData::data.lastCalled = StaticData::LastFn::GET_HANDLE_FN;
    return &this->m_handle;
}

TestRawTime::Status TestRawTime::now() {
    StaticData::data.lastCalled = StaticData::LastFn::GET_TIME_FN;
    return Status::OP_OK;
}

TestRawTime::Status TestRawTime::getTimeInterval(const Os::RawTime& other, Fw::TimeInterval& interval) const {
    StaticData::data.lastCalled = StaticData::LastFn::GET_INTERVAL_FN;
    return Status::OP_OK;
}

Fw::SerializeStatus TestRawTime::serializeTo(Fw::SerialBufferBase& buffer, Fw::Endianness mode) const {
    StaticData::data.lastCalled = StaticData::LastFn::SERIALIZE_FN;
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus TestRawTime::deserializeFrom(Fw::SerialBufferBase& buffer, Fw::Endianness mode) {
    StaticData::data.lastCalled = StaticData::LastFn::DESERIALIZE_FN;
    return Fw::FW_SERIALIZE_OK;
}

}  // namespace Test
}  // namespace RawTime
}  // namespace Stub
}  // namespace Os
