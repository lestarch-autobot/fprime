// ======================================================================
// \title  DpWriterTester.cpp
// \author bocchino
// \brief  cpp file for DpWriter component test harness implementation class
// ======================================================================

#include "DpWriterTester.hpp"
#include "Os/Stub/test/File.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

DpWriterTester ::DpWriterTester()
    : DpWriterGTestBase("DpWriterTester", DpWriterTester::MAX_HISTORY_SIZE), component("DpWriter") {
    this->initComponents();
    this->connectPorts();
    Os::Stub::File::Test::StaticData::data.setNextStatus(Os::File::OP_OK);
    Os::Stub::File::Test::StaticData::data.writeResult = this->abstractState.m_writeResultData;
    Os::Stub::File::Test::StaticData::data.writeResultSize = sizeof(this->abstractState.m_writeResultData);
    Os::Stub::File::Test::StaticData::data.pointer = 0;
}

DpWriterTester ::~DpWriterTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Handlers for typed from ports
// ----------------------------------------------------------------------

void DpWriterTester::from_procBufferSendOut_handler(FwIndexType portNum, Fw::Buffer& buffer) {
    this->pushFromPortEntry_procBufferSendOut(buffer);
    this->abstractState.m_procTypes =
        static_cast<Fw::DpCfg::ProcType::SerialType>(this->abstractState.m_procTypes | (1 << portNum));

    if (this->abstractState.m_procShrinkDataSizeOpt.has_value()) {
        // Update the data size in the container
        const FwSizeType newSize = this->abstractState.m_procShrinkDataSizeOpt.value();
        Fw::DpContainer c(0, buffer);
        c.deserializeHeader();
        c.setDataSize(newSize);
        c.serializeHeader();
    }
}

void DpWriterTester::from_dpWrittenOut_handler(FwIndexType portNum,
                                               const Fw::StringBase& fileName,
                                               FwDpPriorityType priority,
                                               FwSizeType fileSize) {
    this->abstractState.m_dpWrittenOutPortNumOpt = Fw::Optional<FwIndexType>(portNum);
    this->pushFromPortEntry_dpWrittenOut(fileName, priority, fileSize);
}

void DpWriterTester::from_deallocBufferSendOut_handler(FwIndexType portNum, Fw::Buffer& buffer) {
    this->abstractState.m_deallocBufferSendOutPortNumOpt = Fw::Optional<FwIndexType>(portNum);
    this->pushFromPortEntry_deallocBufferSendOut(buffer);
}

// ----------------------------------------------------------------------
// Public member functions
// ----------------------------------------------------------------------

void DpWriterTester::printEvents() {
    this->printTextLogHistory(stdout);
}

// ----------------------------------------------------------------------
// Protected helper functions
// ----------------------------------------------------------------------

Os::File::Status DpWriterTester::pickOsFileError() {
    U32 u32Status = STest::Pick::lowerUpper(Os::File::OP_OK + 1, Os::File::MAX_STATUS - 1);
    return static_cast<Os::File::Status>(u32Status);
}

#define TESTER_CHECK_CHANNEL(NAME)                                           \
    {                                                                        \
        const auto changeStatus = this->abstractState.m_##NAME.updatePrev(); \
        if (changeStatus == TestUtils::OnChangeStatus::CHANGED) {            \
            ASSERT_TLM_##NAME##_SIZE(1);                                     \
            ASSERT_TLM_##NAME(0, this->abstractState.m_##NAME.value);        \
        } else {                                                             \
            ASSERT_TLM_##NAME##_SIZE(0);                                     \
        }                                                                    \
    }

void DpWriterTester::constructDpFileName(FwDpIdType id, const Fw::Time& timeTag, Fw::StringBase& fileName) {
    fileName.format(DP_FILENAME_FORMAT, this->component.m_dpFileNamePrefix.toChar(), id, timeTag.getSeconds(),
                    timeTag.getUSeconds());
}

void DpWriterTester::checkProcTypes(const Fw::DpContainer& container) {
    // The header must record the requested processing types with the override applied
    const Fw::DpCfg::ProcType::SerialType expectedProcTypes =
        this->abstractState.applyProcOverride(this->abstractState.m_requestedProcTypes);
    ASSERT_EQ(container.getProcTypes(), expectedProcTypes);
    // Exactly the recorded processing ports must have been invoked
    U32 expectedNumProcTypes = 0;
    for (FwIndexType i = 0; i < DpWriterNumProcPorts; i++) {
        if ((expectedProcTypes & (1U << i)) != 0) {
            ++expectedNumProcTypes;
        }
    }
    ASSERT_from_procBufferSendOut_SIZE(expectedNumProcTypes);
    ASSERT_EQ(container.getProcTypes(), this->abstractState.m_procTypes);
    // The header hash must cover the recorded processing types
    Utils::HashBuffer storedHash;
    Utils::HashBuffer computedHash;
    ASSERT_EQ(Fw::Success::SUCCESS, container.checkHeaderHash(storedHash, computedHash));
}

void DpWriterTester::checkTelemetry() {
    TESTER_CHECK_CHANNEL(NumBuffersReceived);
    TESTER_CHECK_CHANNEL(NumBytesWritten);
    TESTER_CHECK_CHANNEL(NumSuccessfulWrites);
    TESTER_CHECK_CHANNEL(NumFailedWrites);
    TESTER_CHECK_CHANNEL(NumErrors);
    TESTER_CHECK_CHANNEL(ProcOverrideState);
    TESTER_CHECK_CHANNEL(ProcOverrideMask);
}

// ----------------------------------------------------------------------
// Private helper functions
// ----------------------------------------------------------------------

// connectPorts() and initComponents() are hand-written in place of UT_AUTO_HELPERS so that the last
// processing port stays unconnected; keep them in sync with DpWriter.fpp
void DpWriterTester::connectPorts() {
    // Connect special input ports
    this->connect_to_cmdIn(0, this->component.get_cmdIn_InputPort(0));

    // Connect special output ports
    this->component.set_cmdRegIn_OutputPort(0, this->get_from_cmdRegIn(0));
    this->component.set_cmdResponseOut_OutputPort(0, this->get_from_cmdResponseOut(0));
    this->component.set_eventOut_OutputPort(0, this->get_from_eventOut(0));
#if FW_ENABLE_TEXT_LOGGING == 1
    this->component.set_textEventOut_OutputPort(0, this->get_from_textEventOut(0));
#endif
    this->component.set_timeGetOut_OutputPort(0, this->get_from_timeGetOut(0));
    this->component.set_tlmOut_OutputPort(0, this->get_from_tlmOut(0));

    // Connect typed input ports
    for (FwIndexType i = 0; i < DpWriterNumPorts; i++) {
        this->connect_to_bufferSendIn(i, this->component.get_bufferSendIn_InputPort(i));
    }
    this->connect_to_schedIn(0, this->component.get_schedIn_InputPort(0));

    // Connect typed output ports
    for (FwIndexType i = 0; i < DpWriterNumPorts; i++) {
        this->component.set_deallocBufferSendOut_OutputPort(i, this->get_from_deallocBufferSendOut(i));
        this->component.set_dpWrittenOut_OutputPort(i, this->get_from_dpWrittenOut(i));
    }
    // Leave the last processing port unconnected to exercise override validation
    for (FwIndexType i = 0; i < AbstractState::NUM_CONNECTED_PROC_PORTS; i++) {
        this->component.set_procBufferSendOut_OutputPort(i, this->get_from_procBufferSendOut(i));
    }
}

void DpWriterTester::initComponents() {
    this->init();
    this->component.init(DpWriterTester::TEST_INSTANCE_QUEUE_DEPTH, DpWriterTester::TEST_INSTANCE_ID);
}

void DpWriterTester::doDispatch() {
    this->component.doDispatch();
}

FwIndexType DpWriterTester::getBufferTooSmallForDataThrottleCount() {
    return this->component.DpWriterComponentBase::m_BufferTooSmallForDataThrottle;
}

FwIndexType DpWriterTester::getBufferTooSmallForPacketThrottleCount() {
    return this->component.DpWriterComponentBase::m_BufferTooSmallForPacketThrottle;
}

FwIndexType DpWriterTester::getFileOpenErrorThrottleCount() {
    return this->component.DpWriterComponentBase::m_FileOpenErrorThrottle;
}

FwIndexType DpWriterTester::getFileWriteErrorThrottleCount() {
    return this->component.DpWriterComponentBase::m_FileWriteErrorThrottle;
}

FwIndexType DpWriterTester::getInvalidBufferThrottleCount() {
    return this->component.DpWriterComponentBase::m_InvalidBufferThrottle;
}

FwIndexType DpWriterTester::getInvalidHeaderHashThrottleCount() {
    return this->component.DpWriterComponentBase::m_InvalidHeaderHashThrottle;
}

FwIndexType DpWriterTester::getInvalidHeaderThrottleCount() {
    return this->component.DpWriterComponentBase::m_InvalidHeaderThrottle;
}

}  // namespace Svc
