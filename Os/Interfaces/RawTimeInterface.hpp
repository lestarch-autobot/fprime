// ======================================================================
// \title Os/Interfaces/RawTimeInterface.hpp
// \brief pure-virtual interface implemented by Os::RawTime implementations
// ======================================================================
#ifndef OS_INTERFACES_RAWTIMEINTERFACE_HPP_
#define OS_INTERFACES_RAWTIMEINTERFACE_HPP_

#include <Os/Interfaces/RawTimeUserInterface.hpp>

namespace Os {

//! \brief interface for RawTime implementations
//!
//! Implementations must be copyable and constructible from a `RawTimeSource` (see Os::RawTimeCommon).
class RawTimeInterface : public RawTimeUserInterface {
  public:
    //! \brief default constructor
    RawTimeInterface() = default;

    //! \brief default virtual destructor
    virtual ~RawTimeInterface() = default;

    //! \brief return the underlying RawTime handle (implementation specific)
    //! \return internal RawTime handle representation
    virtual RawTimeHandle* getHandle() = 0;

    //! \brief Get the current time.
    //!
    //! This function retrieves the current time and stores it in the RawTime object.
    //! Each implementation should define its RawTimeHandle type for storing the time.
    //!
    //! \return Status indicating the result of the operation.
    virtual Status now() = 0;

    //! \brief Calculate the time interval between this and another raw time.
    //!
    //! \param other The other raw time to compare against: always the same implementation type as this object.
    //! \param interval Output parameter to store the calculated time interval.
    //! \return Status indicating the result of the operation.
    virtual Status getTimeInterval(const RawTimeInterface& other, Fw::TimeInterval& interval) const = 0;

    //! \brief Serialize the contents of the RawTimeInterface object into a buffer.
    //!
    //! \note The serialization must fit within `FW_RAW_TIME_SERIALIZATION_MAX_SIZE` bytes.
    //!
    //! \param buffer The buffer to serialize the contents into.
    //! \param mode Endianness to use when serializing to buffer.
    //! \return Fw::SerializeStatus indicating the result of the serialization.
    virtual Fw::SerializeStatus serializeTo(Fw::SerialBufferBase& buffer,
                                            Fw::Endianness mode = Fw::Endianness::BIG) const = 0;

    //! \brief Deserialize the contents of the RawTimeInterface object from a buffer.
    //!
    //! \param buffer The buffer to deserialize the contents from.
    //! \param mode Endianness to use when deserializing from the buffer.
    //! \return Fw::SerializeStatus indicating the result of the deserialization.
    virtual Fw::SerializeStatus deserializeFrom(Fw::SerialBufferBase& buffer,
                                                Fw::Endianness mode = Fw::Endianness::BIG) = 0;
};

}  // namespace Os

#endif  // OS_INTERFACES_RAWTIMEINTERFACE_HPP_
