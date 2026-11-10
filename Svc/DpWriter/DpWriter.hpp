// ======================================================================
// \title  DpWriter.hpp
// \author bocchino
// \brief  hpp file for DpWriter component implementation class
// ======================================================================

#ifndef Svc_DpWriter_HPP
#define Svc_DpWriter_HPP

#include <config/DpCfg.hpp>

#include <atomic>
#include <limits>

#include "Fw/Dp/DpContainer.hpp"
#include "Fw/Types/FileNameString.hpp"
#include "Fw/Types/String.hpp"
#include "Fw/Types/SuccessEnumAc.hpp"
#include "Svc/DpWriter/DpWriterComponentAc.hpp"

namespace Svc {

class DpWriter final : public DpWriterComponentBase {
    friend class DpWriterTester;

    // Keep the generated routing arrays consistent with the forwarding contract.
    static_assert(DpWriter::NUM_BUFFERSENDIN_INPUT_PORTS == static_cast<FwSizeType>(DpWriterNumPorts),
                  "Number of buffer send in ports must equal DpWriterNumPorts");
    static_assert(DpWriter::NUM_DPWRITTENOUT_OUTPUT_PORTS == static_cast<FwSizeType>(DpWriterNumPorts),
                  "Number of dp written out ports must equal DpWriterNumPorts");
    static_assert(DpWriter::NUM_DEALLOCBUFFERSENDOUT_OUTPUT_PORTS == static_cast<FwSizeType>(DpWriterNumPorts),
                  "Number of dealloc buffer send out ports must equal DpWriterNumPorts");
    static_assert(DpWriter::NUM_PROCBUFFERSENDOUT_OUTPUT_PORTS <=
                      std::numeric_limits<Fw::DpCfg::ProcType::SerialType>::digits,
                  "Every processing port must have a bit in the ProcType bit mask");
    static_assert(DpWriter::NUM_PROCBUFFERSENDOUT_OUTPUT_PORTS <= std::numeric_limits<U8>::digits,
                  "SET_PROC_OVERRIDE_MASK, the override events, and ProcOverrideMask carry the mask as U8");

    //! Bit mask with one bit set per configured processing port; the static assertions above keep it within U8
    static constexpr Fw::DpCfg::ProcType::SerialType DPWRITER_CONFIGURED_PROC_MASK =
        static_cast<Fw::DpCfg::ProcType::SerialType>((1U << DpWriter::NUM_PROCBUFFERSENDOUT_OUTPUT_PORTS) - 1U);

  public:
    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    //! Construct object DpWriter
    //!
    DpWriter(const char* const compName  //!< The component name
    );

    //! Destroy object DpWriter
    //!
    ~DpWriter();

    //! Configure writer
    void configure(const Fw::ConstStringBase& dpFileNamePrefix  //!< The file name prefix for writing DP files
    );

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for bufferSendIn
    //!
    void bufferSendIn_handler(const FwIndexType portNum,  //!< The port number
                              Fw::Buffer& fwBuffer        //!< The buffer
                              ) final;

    //! Handler implementation for schedIn
    //!
    void schedIn_handler(const FwIndexType portNum,  //!< The port number
                         U32 context                 //!< The call order
                         ) final;

    //! Internal interface handler for schedInInternal
    void schedInInternal_internalInterfaceHandler() final;

    //! Overflow hook for schedInInternal
    void schedInInternal_overflowHook() final;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command CLEAR_EVENT_THROTTLE
    //!
    //! Clear event throttling
    void CLEAR_EVENT_THROTTLE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                         U32 cmdSeq            //!< The command sequence number
                                         ) final;

    //! Handler implementation for command SET_PROC_OVERRIDE_STATE
    //!
    //! Enable or disable the processing override applied to every data product container
    void SET_PROC_OVERRIDE_STATE_cmdHandler(FwOpcodeType opCode,      //!< The opcode
                                            U32 cmdSeq,               //!< The command sequence number
                                            const Fw::Enabled& state  //!< ENABLED applies the override mask
                                            ) final;

    //! Handler implementation for command ADD_PROC_OVERRIDE
    //!
    //! Add a processing type to the processing override mask
    void ADD_PROC_OVERRIDE_cmdHandler(FwOpcodeType opCode,                 //!< The opcode
                                      U32 cmdSeq,                          //!< The command sequence number
                                      const Fw::DpCfg::ProcType& procType  //!< The processing type to add
                                      ) final;

    //! Handler implementation for command REMOVE_PROC_OVERRIDE
    //!
    //! Remove a processing type from the processing override mask
    void REMOVE_PROC_OVERRIDE_cmdHandler(FwOpcodeType opCode,                 //!< The opcode
                                         U32 cmdSeq,                          //!< The command sequence number
                                         const Fw::DpCfg::ProcType& procType  //!< The processing type to remove
                                         ) final;

    //! Handler implementation for command SET_PROC_OVERRIDE_MASK
    //!
    //! Set the processing override mask directly
    void SET_PROC_OVERRIDE_MASK_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                           U32 cmdSeq,           //!< The command sequence number
                                           U8 procTypes          //!< The bit mask of processing types
                                           ) final;

  private:
    // ----------------------------------------------------------------------
    // Private helper functions
    // ----------------------------------------------------------------------

    //! Deserialize the packet header
    //! \return Success or failure
    Fw::Success::T deserializePacketHeader(Fw::Buffer& buffer,         //!< The packet buffer
                                           Fw::DpContainer& container  //!< The container
    );

    //! Validate and store a new processing override mask, emit the event, and send the command response
    void handleProcOverrideMaskCmd(FwOpcodeType opCode,                       //!< The opcode
                                   U32 cmdSeq,                                //!< The command sequence number
                                   Fw::DpCfg::ProcType::SerialType procTypes  //!< The requested mask
    );

    //! Apply the processing override to the processing types requested by a container
    //! \return The processing types to perform: the override mask when ENABLED, else the requested types
    Fw::DpCfg::ProcType::SerialType applyProcOverride(
        Fw::DpCfg::ProcType::SerialType procTypes  //!< The requested processing types
    ) const;

    //! Perform processing on a packet buffer: apply the processing override, route the buffer
    //! through the selected processing ports, and record the effective processing types in the header
    void performProcessing(Fw::DpContainer& container  //!< The container
    );

    //! Write the file
    //! \return Success or failure
    Fw::Success::T writeFile(const Fw::DpContainer& container,    //!< The container (input)
                             const Fw::FileNameString& fileName,  //!< The file name
                             FwSizeType& fileSize                 //!< The file size (output)
    );

    //! Send the DpWritten notification
    void sendNotification(FwIndexType portNum,                 //!< The routing port number
                          const Fw::DpContainer& container,    //!< The container
                          const Fw::FileNameString& fileName,  //!< The file name
                          FwSizeType fileSize                  //!< The file size
    );

  private:
    // ----------------------------------------------------------------------
    // Private member variables
    // ----------------------------------------------------------------------

    //! The number of buffers received
    U32 m_numBuffersReceived = 0;

    //! The number of bytes written
    U64 m_numBytesWritten = 0;

    //! The number of successful writes
    U32 m_numSuccessfulWrites = 0;

    //! The number of failed writes
    U32 m_numFailedWrites = 0;

    //! The number of errors
    U32 m_numErrors = 0;

    //! True while a schedInInternal message is on the queue
    std::atomic<bool> m_schedInQueued{false};

    //! ENABLED replaces the header processing types with m_procOverrideMask; DISABLED leaves them as requested
    Fw::Enabled m_procOverrideState = Fw::Enabled::DISABLED;

    //! The processing types selected for every container while the override is ENABLED
    Fw::DpCfg::ProcType::SerialType m_procOverrideMask = 0;

    //! The file name prefix for writing DP files
    //! The precise meaning depends on the DP format string
    //! For example, this could be a directory path prefix
    Fw::FileNameString m_dpFileNamePrefix;
};

}  // end namespace Svc

#endif
