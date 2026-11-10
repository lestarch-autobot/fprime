// ======================================================================
// \title  DpWriterTestMain.cpp
// \author bocchino
// \brief  cpp file for DpWriter component test main function
// ======================================================================

#include "Fw/Test/UnitTest.hpp"
#include "STest/Random/Random.hpp"
#include "Svc/DpWriter/test/ut/Rules/Testers.hpp"
#include "Svc/DpWriter/test/ut/Scenarios/Random.hpp"

namespace Svc {

TEST(BufferSendIn, BufferTooSmallForData) {
    COMMENT("Invoke bufferSendIn with a buffer that is too small to hold the data size specified in the header.");
    REQUIREMENT("SVC-DPMANAGER-001");
    BufferSendIn::Tester tester;
    tester.BufferTooSmallForData();
}

TEST(BufferSendIn, BufferTooSmallForPacket) {
    COMMENT("Invoke bufferSendIn with a buffer that is too small to hold a data product packet.");
    REQUIREMENT("SVC-DPMANAGER-001");
    BufferSendIn::Tester tester;
    tester.BufferTooSmallForPacket();
}

TEST(BufferSendIn, FileNameFormatError) {
    COMMENT("Invoke bufferSendIn with a file name format error and verify the event throttle.");
    REQUIREMENT("SVC-DPMANAGER-001");
    BufferSendIn::Tester tester;
    tester.FileNameFormatError();
}

TEST(BufferSendIn, FileOpenError) {
    COMMENT("Invoke bufferSendIn with a file open error.");
    REQUIREMENT("SVC-DPMANAGER-001");
    BufferSendIn::Tester tester;
    tester.FileOpenError();
}

TEST(BufferSendIn, FileWriteError) {
    COMMENT("Invoke bufferSendIn with a file write error.");
    REQUIREMENT("SVC-DPMANAGER-001");
    BufferSendIn::Tester tester;
    tester.FileWriteError();
}

TEST(BufferSendIn, InvalidBuffer) {
    COMMENT("Invoke bufferSendIn with an invalid buffer.");
    REQUIREMENT("SVC-DPMANAGER-001");
    BufferSendIn::Tester tester;
    tester.InvalidBuffer();
}

TEST(BufferSendIn, InvalidHeader) {
    COMMENT("Invoke bufferSendIn with an invalid packet header.");
    REQUIREMENT("SVC-DPMANAGER-001");
    BufferSendIn::Tester tester;
    tester.InvalidHeader();
}

TEST(BufferSendIn, InvalidHeaderHash) {
    COMMENT("Invoke bufferSendIn with a buffer that has an invalid header hash.");
    REQUIREMENT("SVC-DPMANAGER-001");
    BufferSendIn::Tester tester;
    tester.InvalidHeaderHash();
}

TEST(BufferSendIn, OK) {
    COMMENT("Invoke bufferSendIn with nominal input.");
    REQUIREMENT("SVC-DPMANAGER-001");
    REQUIREMENT("SVC-DPMANAGER-002");
    REQUIREMENT("SVC-DPMANAGER-003");
    REQUIREMENT("SVC-DPMANAGER-004");
    REQUIREMENT("SVC-DPMANAGER-005");
    BufferSendIn::Tester tester;
    tester.OK();
}

TEST(BufferSendIn, OKProcShrink) {
    COMMENT("Invoke bufferSendIn with nominal input. Shrink the buffer in processing");
    REQUIREMENT("SVC-DPMANAGER-002");
    REQUIREMENT("SVC-DPMANAGER-003");
    REQUIREMENT("SVC-DPMANAGER-004");
    REQUIREMENT("SVC-DPMANAGER-007");
    BufferSendIn::Tester tester;
    tester.OKProcShrink();
}

TEST(BufferSendIn, RoutingPorts) {
    COMMENT("Verify fused DpWriter routing paths preserve the input port index.");
    REQUIREMENT("SVC-DPWRITER-001");
    REQUIREMENT("SVC-DPWRITER-005");
    REQUIREMENT("SVC-DPWRITER-008");
    BufferSendIn::Tester tester;
    tester.RoutingPorts();
}

TEST(CLEAR_EVENT_THROTTLE, OK) {
    COMMENT("Test the CLEAR_EVENT_THROTTLE command.");
    REQUIREMENT("SVC-DPMANAGER-006");
    CLEAR_EVENT_THROTTLE::Tester tester;
    tester.OK();
}

TEST(FileOpenStatus, Error) {
    COMMENT("Set the file open status to an error value.");
    FileOpenStatus::Tester tester;
    tester.Error();
}

TEST(FileOpenStatus, OK) {
    COMMENT("Set the file open status to OP_OK.");
    FileOpenStatus::Tester tester;
    tester.OK();
}

TEST(FileWriteStatus, Error) {
    COMMENT("Set the file write status to an error value.");
    FileWriteStatus::Tester tester;
    tester.Error();
}

TEST(FileWriteStatus, OK) {
    COMMENT("Set the file write status to OP_OK.");
    FileWriteStatus::Tester tester;
    tester.OK();
}

TEST(Scenarios, Random) {
    COMMENT("Random scenario with all rules.");
    REQUIREMENT("SVC-DPMANAGER-001");
    REQUIREMENT("SVC-DPMANAGER-002");
    REQUIREMENT("SVC-DPMANAGER-003");
    REQUIREMENT("SVC-DPMANAGER-004");
    REQUIREMENT("SVC-DPMANAGER-005");
    REQUIREMENT("SVC-DPMANAGER-006");
    const FwSizeType numSteps = 10000;
    Scenarios::Random::Tester tester;
    tester.run(numSteps);
}

TEST(SET_PROC_OVERRIDE, OK) {
    COMMENT("Set a valid processing override and verify the response, event, and telemetry.");
    REQUIREMENT("SVC-DPWRITER-009");
    REQUIREMENT("SVC-DPWRITER-011");
    SET_PROC_OVERRIDE::Tester tester;
    tester.OK();
}

TEST(SET_PROC_OVERRIDE, Invalid) {
    COMMENT("Reject a processing override that selects an unconnected or nonexistent processing port.");
    REQUIREMENT("SVC-DPWRITER-010");
    SET_PROC_OVERRIDE::Tester tester;
    tester.Invalid();
}

TEST(SET_PROC_OVERRIDE, InvalidUnconnected) {
    COMMENT("Reject forcing on an existing processing type whose port is unconnected.");
    REQUIREMENT("SVC-DPWRITER-010");
    SET_PROC_OVERRIDE::Tester tester;
    tester.InvalidUnconnected();
}

TEST(SET_PROC_OVERRIDE, ForceOn) {
    COMMENT("Enable an override selecting every connected processing port and verify each is invoked and recorded.");
    REQUIREMENT("SVC-DPWRITER-009");
    SET_PROC_OVERRIDE::Tester tester;
    tester.ForceOn();
}

TEST(SET_PROC_OVERRIDE, ForceOff) {
    COMMENT("Enable an empty override and verify no processing port is invoked and the header is cleared.");
    REQUIREMENT("SVC-DPWRITER-009");
    SET_PROC_OVERRIDE::Tester tester;
    tester.ForceOff();
}

TEST(SET_PROC_OVERRIDE, Clear) {
    COMMENT("Disable the override and verify header-controlled processing resumes regardless of the mask.");
    REQUIREMENT("SVC-DPWRITER-009");
    SET_PROC_OVERRIDE::Tester tester;
    tester.Clear();
}

TEST(SchedIn, OK) {
    COMMENT("Invoke schedIn with nominal input.");
    REQUIREMENT("SVC-DPMANAGER-006");
    SchedIn::Tester tester;
    tester.OK();
}

}  // namespace Svc

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    STest::Random::seed();
    return RUN_ALL_TESTS();
}
