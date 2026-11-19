// ======================================================================
// \title Os/DelegateRawTime.cpp
// \brief link-time delegate implementation: Os::DelegateRawTime
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <Os/DelegateRawTime.hpp>
#include <Os/RawTime.hpp>

namespace Os {

DelegateRawTime::DelegateRawTime() : m_delegate(*RawTimeInterface::getDelegate(m_handle_storage)) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<RawTimeInterface*>(&this->m_handle_storage[0]));
}

DelegateRawTime::DelegateRawTime(RawTimeSource source)
    : m_delegate(*RawTimeInterface::getDelegate(m_handle_storage, nullptr, source)), m_source(source) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<RawTimeInterface*>(&this->m_handle_storage[0]));
}

DelegateRawTime::~DelegateRawTime() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<RawTimeInterface*>(&this->m_handle_storage[0]));
    m_delegate.~RawTimeInterface();
}

// m_handle_storage is placement-new storage populated by getDelegate below
// cppcheck-suppress missingMemberCopy
DelegateRawTime::DelegateRawTime(const DelegateRawTime& other)
    : m_delegate(*RawTimeInterface::getDelegate(m_handle_storage, &other.m_delegate, other.m_source)),
      m_source(other.m_source) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<RawTimeInterface*>(&this->m_handle_storage[0]));
}

DelegateRawTime& DelegateRawTime::operator=(const DelegateRawTime& other) {
    if (this != &other) {
        this->m_source = other.m_source;
        this->m_delegate = *RawTimeInterface::getDelegate(m_handle_storage, &other.m_delegate, other.m_source);
    }
    return *this;
}

RawTimeHandle* DelegateRawTime::getHandle() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<RawTimeInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getHandle();
}

DelegateRawTime::Status DelegateRawTime::now() {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<RawTimeInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.now();
}

DelegateRawTime::Status DelegateRawTime::getTimeInterval(const Os::RawTime& other, Fw::TimeInterval& result) const {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const RawTimeInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getTimeInterval(other, result);
}

Fw::SerializeStatus DelegateRawTime::serializeTo(Fw::SerialBufferBase& buffer, Fw::Endianness mode) const {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const RawTimeInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.serializeTo(buffer, mode);
}

Fw::SerializeStatus DelegateRawTime::deserializeFrom(Fw::SerialBufferBase& buffer, Fw::Endianness mode) {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const RawTimeInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.deserializeFrom(buffer, mode);
}

DelegateRawTime::Status DelegateRawTime::getDiffUsec(const RawTime& other, U32& result) const {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const RawTimeInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate.getDiffUsec(other, result);
}

bool DelegateRawTime::operator==(const RawTime& other) const {
    FW_ASSERT(&this->m_delegate == reinterpret_cast<const RawTimeInterface*>(&this->m_handle_storage[0]));
    return this->m_delegate == other;
}

RawTimeSource DelegateRawTime::getSource() const {
    return this->m_source;
}

}  // namespace Os
