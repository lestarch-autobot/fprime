// ======================================================================
// \title  AbstractState.hpp
// \author Rob Bocchino
// \brief  Header file for abstract state
//
// \copyright
// Copyright (C) 2024 California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government sponsorship
// acknowledged.
// ======================================================================

#ifndef Svc_AbstractState_HPP
#define Svc_AbstractState_HPP

#include <cstring>

#include <Fw/Types/Optional.hpp>
#include "Fw/Types/Assert.hpp"
#include "Os/File.hpp"
#include "STest/Pick/Pick.hpp"
#include "Svc/DpWriter/DpWriter.hpp"
#include "TestUtils/OnChangeChannel.hpp"
#include "config/FppConstantsAc.hpp"

namespace Svc {

class AbstractState {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    //! The minimum data size
    static constexpr FwSizeType MIN_DATA_SIZE = 0;

    //! The maximum data size
    static constexpr FwSizeType MAX_DATA_SIZE = 1024;

    //! The maximum buffer size
    static constexpr FwSizeType MAX_BUFFER_SIZE = Fw::DpContainer::getPacketSizeForDataSize(MAX_DATA_SIZE);

  public:
    // ----------------------------------------------------------------------
    // Constructors
    // ----------------------------------------------------------------------

    //! Construct an AbstractState object
    AbstractState()
        : m_dataSizeOpt(),
          m_NumBuffersReceived(0),
          m_NumBytesWritten(0),
          m_NumFailedWrites(0),
          m_NumSuccessfulWrites(0),
          m_NumErrors(0),
          m_ProcOverrideState(Fw::Enabled::DISABLED),
          m_ProcOverrideMask(0),
          m_procTypes(0),
          m_requestedProcTypes(0),
          m_modelProcOverrideState(Fw::Enabled::DISABLED),
          m_modelProcOverrideMask(0),
          m_nextProcOverrideStateOpt(),
          m_nextProcOverrideMaskOpt(),
          m_nextProcTypeOpt(),
          m_procShrinkDataSizeOpt(),
          m_dpWrittenOutPortNumOpt(),
          m_deallocBufferSendOutPortNumOpt() {}

  public:
    // ----------------------------------------------------------------------
    // Public member functions
    // ----------------------------------------------------------------------

    //! Get the data size
    FwSizeType getDataSize() const {
        return this->m_dataSizeOpt.value_or(STest::Pick::lowerUpper(MIN_DATA_SIZE, MAX_DATA_SIZE));
    }

    //! Set the data size
    void setDataSize(FwSizeType dataSize) { this->m_dataSizeOpt = dataSize; }

    void clearDataSize() { this->m_dataSizeOpt.reset(); }

    //! Get a data product buffer backed by bufferData
    //! \return The buffer
    Fw::Buffer getDpBuffer();

    //! Get a data product buffer backed by bufferData
    //! \return The buffer
    Fw::Buffer getDpBufferWithProc(Fw::DpCfg::ProcType::SerialType procTypes);

    //! Get the bit mask selecting every processing port
    //! \return The bit mask
    static Fw::DpCfg::ProcType::SerialType getAllProcMask();

    //! Get the mask with the low numBits bits set
    static Fw::DpCfg::ProcType::SerialType getLowBitsMask(FwIndexType numBits  //!< The number of bits
    );

    //! Apply the modeled processing override to requested processing types
    //! \return The processing types expected to be performed
    Fw::DpCfg::ProcType::SerialType applyProcOverride(
        Fw::DpCfg::ProcType::SerialType procTypes  //!< The requested processing types
    ) const;

  private:
    // ----------------------------------------------------------------------
    // Private state variables
    // ----------------------------------------------------------------------

    //! The current buffer size
    Fw::Optional<FwSizeType> m_dataSizeOpt;

  public:
    // ----------------------------------------------------------------------
    // Public state variables
    // ----------------------------------------------------------------------

    //! The number of buffers received
    TestUtils::OnChangeChannel<U32> m_NumBuffersReceived;

    //! The number of bytes written
    TestUtils::OnChangeChannel<U64> m_NumBytesWritten;

    //! The number of failed writes
    TestUtils::OnChangeChannel<U32> m_NumFailedWrites;

    //! The number of successful writes
    TestUtils::OnChangeChannel<U32> m_NumSuccessfulWrites;

    //! The number of errors
    TestUtils::OnChangeChannel<U32> m_NumErrors;

    //! The number of BufferTooSmallForData events since the last throttle clear
    FwSizeType m_bufferTooSmallForDataEventCount = 0;

    //! The number of BufferTooSmallForPacket events since the last throttle clear
    FwSizeType m_bufferTooSmallForPacketEventCount = 0;

    //! The number of buffer invalid events since the last throttle clear
    FwSizeType m_invalidBufferEventCount = 0;

    //! The number of file open error events since the last throttle clear
    FwSizeType m_fileOpenErrorEventCount = 0;

    //! The number of file write error events since the last throttle clear
    FwSizeType m_fileWriteErrorEventCount = 0;

    //! The number of invalid header hash events since the last throttle clear
    FwSizeType m_invalidHeaderHashEventCount = 0;

    //! The number of invalid header events since the last throttle clear
    FwSizeType m_invalidHeaderEventCount = 0;

    //! Data for buffers
    U8 m_bufferData[MAX_BUFFER_SIZE] = {};

    //! Data for write results
    U8 m_writeResultData[MAX_BUFFER_SIZE] = {};

    //! The processing override state channel
    TestUtils::OnChangeChannel<Fw::Enabled::T> m_ProcOverrideState;

    //! The processing override mask channel
    TestUtils::OnChangeChannel<U8> m_ProcOverrideMask;

    //! Bit mask for processing out port calls
    Fw::DpCfg::ProcType::SerialType m_procTypes;

    //! The processing types requested in the header of the last generated buffer
    Fw::DpCfg::ProcType::SerialType m_requestedProcTypes;

    //! The modeled processing override state
    Fw::Enabled m_modelProcOverrideState;

    //! The modeled processing override mask
    Fw::DpCfg::ProcType::SerialType m_modelProcOverrideMask;

    //! The state to request in the next ProcOverride::State rule, if any
    Fw::Optional<Fw::Enabled::T> m_nextProcOverrideStateOpt;

    //! The mask to request in the next ProcOverride::MaskOK or MaskInvalid rule, if any
    Fw::Optional<Fw::DpCfg::ProcType::SerialType> m_nextProcOverrideMaskOpt;

    //! The processing type to request in the next ProcOverride::Add or Remove rule, if any
    Fw::Optional<Fw::DpCfg::ProcType::T> m_nextProcTypeOpt;

    Fw::Optional<FwSizeType> m_procShrinkDataSizeOpt;

    //! The last routing port used for DpWritten notification
    Fw::Optional<FwIndexType> m_dpWrittenOutPortNumOpt;

    //! The last routing port used to return a buffer
    Fw::Optional<FwIndexType> m_deallocBufferSendOutPortNumOpt;
};

}  // namespace Svc

#endif
