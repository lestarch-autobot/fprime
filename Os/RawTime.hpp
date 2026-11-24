// ======================================================================
// \title Os/RawTime.hpp
// \brief common definitions for Os::RawTime
// ======================================================================
#ifndef OS_RAWTIME_HPP_
#define OS_RAWTIME_HPP_

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Time/TimeInterval.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Os/Os.hpp>
#include <limits>
#include "config/OsSelection.hpp"  // defines the Os::RawTime alias and OS_RAW_TIME_HEADER; do not remove
#include "config/RawTimeSource.hpp"

namespace Os {

struct RawTimeHandle {};

//! \brief types shared by Os::RawTime users and implementations
struct RawTimeUserInterface {
    // Serialization size for RawTime objects, configured in config/FpConfig.h
    static const FwSizeType SERIALIZED_SIZE = FW_RAW_TIME_SERIALIZATION_MAX_SIZE;

    enum Status {
        OP_OK,           //!< Operation was successful
        OP_OVERFLOW,     //!< Operation result caused an overflow
        INVALID_PARAMS,  //!< Parameters invalid for current platform
        NOT_SUPPORTED,   //!< RawTime does not support operation
        OTHER_ERROR      //!< All other errors
    };
};

//! \brief Os::RawTime wrapper around the implementation `T` selected at compile time
//!
//! Holds the implementation by value and forwards every operation to it. `Os::RawTime` is this class instantiated on
//! the implementation selected in config/OsSelection.hpp. `T` must be copyable and constructible from a
//! `RawTimeSource`.
template <typename T>
class RawTimeCommon final : public RawTimeUserInterface, public Fw::Serializable {
  public:
    using RawTimeUserInterface::SERIALIZED_SIZE;

    RawTimeCommon() = default;  //!<  Default constructor

    //! \brief Constructor with timer source selection
    //! \param source Timer source to use
    explicit RawTimeCommon(RawTimeSource source);

    ~RawTimeCommon() = default;  //!<  Destructor

    //! \brief copy constructor that copies the internal representation
    RawTimeCommon(const RawTimeCommon& other) = default;

    //! \brief assignment operator that copies the internal representation
    RawTimeCommon& operator=(const RawTimeCommon& other) = default;

    //! \brief return the underlying RawTime handle (implementation specific)
    //! \return internal RawTime handle representation
    RawTimeHandle* getHandle();

    //! \brief Get the current time.
    //!
    //! This function retrieves the current time and stores it in the RawTime object.
    //!
    //! \return Status indicating the result of the operation.
    Status now();

    //! \brief Calculate the time interval between this and another raw time.
    //!
    //! This function calculates the time interval between the current raw time and another
    //! specified raw time. The result is stored in the provided (output) interval object.
    //!
    //! \param other The other RawTime to compare against.
    //! \param interval Output parameter to store the calculated time interval.
    //! \return Status indicating the result of the operation.
    Status getTimeInterval(const RawTimeCommon& other, Fw::TimeInterval& interval) const;

    //! \brief Serialize the contents of the RawTime object into a buffer.
    //!
    //! \note The serialization must fit within `FW_RAW_TIME_SERIALIZATION_MAX_SIZE` bytes. This value is
    //! defined in FpConfig.h. For example, Posix systems use a pair of U32 (sec, nanosec) and can therefore
    //! serialize in 8 bytes. Should an OSAL implementation require more than this, the project must increase
    //! that value in its config/ folder.
    //!
    //! \param buffer The buffer to serialize the contents into.
    //! \param mode Endianness to use when serializing to buffer.
    //! \return Fw::SerializeStatus indicating the result of the serialization.
    Fw::SerializeStatus serializeTo(Fw::SerialBufferBase& buffer,
                                    Fw::Endianness mode = Fw::Endianness::BIG) const override;

    //! \brief Deserialize the contents of the RawTime object from a buffer.
    //!
    //! \param buffer The buffer to deserialize the contents from.
    //! \param mode Endianness to use when deserializing from the buffer.
    //! \return Fw::SerializeStatus indicating the result of the deserialization.
    Fw::SerializeStatus deserializeFrom(Fw::SerialBufferBase& buffer,
                                        Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Calculate the difference in microseconds between two RawTime objects.
    //!
    //! \warning This function will return Status::OP_OVERFLOW if the time difference is too large to fit in a U32.
    //! \warning This means the largest time difference that can be measured is 2^32 microseconds (about 71 minutes).
    //! \warning Users should prefer getTimeInterval() for larger intervals.
    //!
    //! \param other The other RawTime object to compare against.
    //! \param result A reference to a U32 variable where the result will be stored.
    //! \return Status indicating the result of the operation.
    Status getDiffUsec(const RawTimeCommon& other, U32& result) const;

    //! \brief Compare whether two RawTime objects are the same (i.e. refer to the same microsecond)
    bool operator==(const RawTimeCommon& other) const;

    //! \brief Get the timer source used by this RawTime instance
    //! \return The RawTimeSource value configured for this instance
    RawTimeSource getSource() const;

  private:
    T m_delegate;                              //!< Delegate for the real implementation
    RawTimeSource m_source = RAWTIME_DEFAULT;  //!< Timer source selection
};

}  // namespace Os

// Validate that OS_RAW_TIME_HEADER was defined by config/OsSelection.hpp
#ifndef OS_RAW_TIME_HEADER
#error "OS_RAW_TIME_HEADER must be defined in config/OsSelection.hpp"
#endif

#include OS_RAW_TIME_HEADER

namespace Os {

template <typename T>
RawTimeCommon<T>::RawTimeCommon(RawTimeSource source) : m_delegate(source), m_source(source) {}

template <typename T>
RawTimeHandle* RawTimeCommon<T>::getHandle() {
    return this->m_delegate.getHandle();
}

template <typename T>
typename RawTimeCommon<T>::Status RawTimeCommon<T>::now() {
    return this->m_delegate.now();
}

template <typename T>
typename RawTimeCommon<T>::Status RawTimeCommon<T>::getTimeInterval(const RawTimeCommon& other,
                                                                    Fw::TimeInterval& interval) const {
    return this->m_delegate.getTimeInterval(other.m_delegate, interval);
}

template <typename T>
Fw::SerializeStatus RawTimeCommon<T>::serializeTo(Fw::SerialBufferBase& buffer, Fw::Endianness mode) const {
    return this->m_delegate.serializeTo(buffer, mode);
}

template <typename T>
Fw::SerializeStatus RawTimeCommon<T>::deserializeFrom(Fw::SerialBufferBase& buffer, Fw::Endianness mode) {
    return this->m_delegate.deserializeFrom(buffer, mode);
}

template <typename T>
typename RawTimeCommon<T>::Status RawTimeCommon<T>::getDiffUsec(const RawTimeCommon& other, U32& result) const {
    Fw::TimeInterval interval;
    Status status = this->getTimeInterval(other, interval);
    if (status != Status::OP_OK) {
        return status;
    }
    U32 seconds = interval.getSeconds();
    U32 useconds = interval.getUSeconds();
    if (seconds > (std::numeric_limits<U32>::max() / 1000000)) {
        result = std::numeric_limits<U32>::max();
        return Status::OP_OVERFLOW;
    }
    U32 secToUsec = seconds * 1000000;
    if (secToUsec > (std::numeric_limits<U32>::max() - useconds)) {
        result = std::numeric_limits<U32>::max();
        return Status::OP_OVERFLOW;
    }
    result = secToUsec + useconds;
    return status;
}

template <typename T>
bool RawTimeCommon<T>::operator==(const RawTimeCommon& other) const {
    Fw::TimeInterval interval;
    Status status = this->getTimeInterval(other, interval);
    return status == Status::OP_OK && interval.getSeconds() == 0 && interval.getUSeconds() == 0;
}

template <typename T>
RawTimeSource RawTimeCommon<T>::getSource() const {
    return this->m_source;
}

}  // namespace Os

#endif  // OS_RAWTIME_HPP_
