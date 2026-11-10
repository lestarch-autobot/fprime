// ======================================================================
// \title  DrainLimit.cpp
// \author lestarch-autobot
// \brief  Test the drain limit (pipelined draining)
//
// \copyright
// Copyright (c) 2026 California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include "DrainLimit.hpp"

namespace Svc {

namespace DrainLimit {

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void BufferAccumulatorTester ::DefaultLimit() {
    this->component.allocateQueue(0, this->buffAccumMallocator, MAX_NUM_BUFFERS, BufferAccumulator_OpState::DRAIN);
    this->doAllocateQueue = true;
    ASSERT_TLM_SIZE(0);

    this->component.preamble();
    ASSERT_TLM_BA_DrainLimit_SIZE(1);
    ASSERT_TLM_BA_DrainLimit(0, 1u);
    ASSERT_TLM_BA_NumOutstandingBuffers_SIZE(1);
    ASSERT_TLM_BA_NumOutstandingBuffers(0, 0u);
    ASSERT_TLM_BA_NumQueuedBuffers_SIZE(1);
    ASSERT_TLM_BA_NumQueuedBuffers(0, 0u);

    // Limit 1: one buffer in flight, the next waits for the return
    this->fill(2);
    ASSERT_from_bufferSendOutDrain_SIZE(1);
    ASSERT_from_bufferSendOutDrain(0, this->m_buffers[0]);
    ASSERT_EQ(1u, this->lastOutstanding());
    this->returnBuffer(0);
    ASSERT_from_bufferSendOutReturn_SIZE(1);
    ASSERT_from_bufferSendOutReturn(0, this->m_buffers[0]);
    ASSERT_from_bufferSendOutDrain_SIZE(2);
    ASSERT_from_bufferSendOutDrain(1, this->m_buffers[1]);
}

void BufferAccumulatorTester ::InitialLimit() {
    const U32 limit = 4;
    const U32 numBuffers = 6;
    this->allocate(BufferAccumulator_OpState::ACCUMULATE, limit);
    this->component.preamble();
    ASSERT_TLM_BA_DrainLimit_SIZE(1);
    ASSERT_TLM_BA_DrainLimit(0, limit);

    this->fill(numBuffers);
    ASSERT_from_bufferSendOutDrain_SIZE(0);

    // Entering DRAIN sends a full window, oldest first
    this->sendCmd_BA_SetMode(0, 0, BufferAccumulator_OpState::DRAIN);
    this->doDispatch();
    ASSERT_CMD_RESPONSE(0, BufferAccumulator::OPCODE_BA_SETMODE, 0, Fw::CmdResponse::OK);
    ASSERT_from_bufferSendOutDrain_SIZE(limit);
    for (U32 i = 0; i < limit; ++i) {
        ASSERT_from_bufferSendOutDrain(i, this->m_buffers[i]);
    }
    ASSERT_EQ(limit, this->lastOutstanding());
    ASSERT_EQ(numBuffers - limit, this->lastQueued());

    // Each return opens one slot
    for (U32 i = 0; i < numBuffers; ++i) {
        this->returnBuffer(i);
        ASSERT_from_bufferSendOutReturn_SIZE(i + 1);
        ASSERT_from_bufferSendOutReturn(i, this->m_buffers[i]);
        const U32 expectedSent = FW_MIN(numBuffers, i + 1 + limit);
        ASSERT_from_bufferSendOutDrain_SIZE(expectedSent);
        ASSERT_from_bufferSendOutDrain(expectedSent - 1, this->m_buffers[expectedSent - 1]);
        ASSERT_EQ(expectedSent - (i + 1), this->lastOutstanding());
    }
    ASSERT_EQ(0u, this->lastOutstanding());
}

void BufferAccumulatorTester ::InitialLimitInvalid() {
    Fw::MallocAllocator allocator;
    ASSERT_DEATH_IF_SUPPORTED(
        this->component.allocateQueue(0, allocator, MAX_NUM_BUFFERS, BufferAccumulator_OpState::ACCUMULATE, 0),
        "Assert: \".*BufferAccumulator.cpp:[0-9]+\" 0");
    ASSERT_DEATH_IF_SUPPORTED(this->component.allocateQueue(0, allocator, MAX_NUM_BUFFERS,
                                                            BufferAccumulator_OpState::ACCUMULATE, MAX_NUM_BUFFERS + 1),
                              "Assert: \".*BufferAccumulator.cpp:[0-9]+\" 11 10");
}

void BufferAccumulatorTester ::SetLimitOK() {
    // DRAIN mode, limit 1: one buffer in flight
    this->fill(3);
    ASSERT_from_bufferSendOutDrain_SIZE(1);

    // Raising the limit in DRAIN mode immediately sends the newly admitted buffers
    this->sendCmd_BA_SetDrainLimit(0, 0, 3);
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, BufferAccumulator::OPCODE_BA_SETDRAINLIMIT, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BA_DrainLimitSet_SIZE(1);
    ASSERT_EVENTS_BA_DrainLimitSet(0, 3u);
    ASSERT_TLM_BA_DrainLimit_SIZE(1);
    ASSERT_TLM_BA_DrainLimit(0, 3u);
    ASSERT_from_bufferSendOutDrain_SIZE(3);
    for (U32 i = 0; i < 3; ++i) {
        ASSERT_from_bufferSendOutDrain(i, this->m_buffers[i]);
    }
    ASSERT_EQ(3u, this->lastOutstanding());

    // Accepted in ACCUMULATE as well, including the maximum value
    this->sendCmd_BA_SetMode(0, 0, BufferAccumulator_OpState::ACCUMULATE);
    this->doDispatch();
    this->sendCmd_BA_SetDrainLimit(0, 0, MAX_NUM_BUFFERS);
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(3);
    ASSERT_CMD_RESPONSE(2, BufferAccumulator::OPCODE_BA_SETDRAINLIMIT, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BA_DrainLimitSet_SIZE(2);
    ASSERT_EVENTS_BA_DrainLimitSet(1, MAX_NUM_BUFFERS);
    ASSERT_TLM_BA_DrainLimit_SIZE(2);
    ASSERT_TLM_BA_DrainLimit(1, MAX_NUM_BUFFERS);
    // No sends in ACCUMULATE
    ASSERT_from_bufferSendOutDrain_SIZE(3);
}

void BufferAccumulatorTester ::SetLimitInvalid() {
    this->sendCmd_BA_SetDrainLimit(0, 0, 0);
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, BufferAccumulator::OPCODE_BA_SETDRAINLIMIT, 0, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_BA_InvalidDrainLimit_SIZE(1);
    ASSERT_EVENTS_BA_InvalidDrainLimit(0, 0u, MAX_NUM_BUFFERS);

    this->sendCmd_BA_SetDrainLimit(0, 0, MAX_NUM_BUFFERS + 1);
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(1, BufferAccumulator::OPCODE_BA_SETDRAINLIMIT, 0, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_BA_InvalidDrainLimit_SIZE(2);
    ASSERT_EVENTS_BA_InvalidDrainLimit(1, MAX_NUM_BUFFERS + 1, MAX_NUM_BUFFERS);

    ASSERT_EVENTS_BA_DrainLimitSet_SIZE(0);
    ASSERT_TLM_BA_DrainLimit_SIZE(0);

    // The limit is still 1: the other buffers wait for the first to return
    this->fill(3);
    ASSERT_from_bufferSendOutDrain_SIZE(1);
    ASSERT_from_bufferSendOutDrain(0, this->m_buffers[0]);

    // Rejected in ACCUMULATE mode too, and the limit is still unchanged (REQ-BA-DL-005)
    this->sendCmd_BA_SetMode(0, 0, BufferAccumulator_OpState::ACCUMULATE);
    this->doDispatch();
    this->sendCmd_BA_SetDrainLimit(0, 0, 0);
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(4);
    ASSERT_CMD_RESPONSE(3, BufferAccumulator::OPCODE_BA_SETDRAINLIMIT, 0, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_BA_InvalidDrainLimit_SIZE(3);
    ASSERT_EVENTS_BA_InvalidDrainLimit(2, 0u, MAX_NUM_BUFFERS);
    ASSERT_TLM_BA_DrainLimit_SIZE(0);
    this->returnBuffer(0);
    ASSERT_from_bufferSendOutDrain_SIZE(1);
    // Back in DRAIN with two queued, exactly one goes out: the limit is still 1
    this->sendCmd_BA_SetMode(0, 0, BufferAccumulator_OpState::DRAIN);
    this->doDispatch();
    ASSERT_from_bufferSendOutDrain_SIZE(2);
    ASSERT_EQ(1u, this->lastQueued());
}

void BufferAccumulatorTester ::WindowedDrain() {
    const U32 limit = 3;
    this->allocate(BufferAccumulator_OpState::ACCUMULATE, limit);
    this->fill(MAX_NUM_BUFFERS);
    ASSERT_from_bufferSendOutDrain_SIZE(0);

    this->sendCmd_BA_SetMode(0, 0, BufferAccumulator_OpState::DRAIN);
    this->doDispatch();
    ASSERT_from_bufferSendOutDrain_SIZE(limit);
    ASSERT_EQ(limit, this->lastOutstanding());

    for (U32 i = 0; i < MAX_NUM_BUFFERS; ++i) {
        this->returnBuffer(i);
        ASSERT_from_bufferSendOutReturn_SIZE(i + 1);
        ASSERT_from_bufferSendOutReturn(i, this->m_buffers[i]);
        const U32 expectedSent = FW_MIN(MAX_NUM_BUFFERS, i + 1 + limit);
        ASSERT_from_bufferSendOutDrain_SIZE(expectedSent);
        ASSERT_EQ(expectedSent - (i + 1), this->lastOutstanding());
    }
    for (U32 i = 0; i < MAX_NUM_BUFFERS; ++i) {
        ASSERT_from_bufferSendOutDrain(i, this->m_buffers[i]);
    }
    ASSERT_EQ(0u, this->lastOutstanding());
    ASSERT_EQ(0u, this->lastQueued());
    // Written on net change only: 0->3 on entering DRAIN, nothing for the seven returns that re-send, then 2, 1, 0
    ASSERT_TLM_BA_NumOutstandingBuffers_SIZE(4);
    ASSERT_TLM_BA_NumOutstandingBuffers(0, limit);
    ASSERT_TLM_BA_NumOutstandingBuffers(3, 0u);
}

void BufferAccumulatorTester ::WindowedPartialDrain() {
    const U32 limit = 4;
    const U32 numBuffers = 6;
    const U32 numToDrain = 5;
    this->allocate(BufferAccumulator_OpState::ACCUMULATE, limit);
    this->fill(numBuffers);

    // A full window goes out; the target is not yet reached so the command stays pending
    this->sendCmd_BA_DrainBuffers(0, 0, numToDrain, BufferAccumulator_BlockMode::BLOCK);
    this->doDispatch();
    ASSERT_from_bufferSendOutDrain_SIZE(limit);
    ASSERT_CMD_RESPONSE_SIZE(0);
    ASSERT_EVENTS_BA_PartialDrainDone_SIZE(0);
    ASSERT_EVENTS_BA_DrainStalled_SIZE(0);

    // The first return sends the fifth buffer, which completes the drain on SEND, not return
    this->returnBuffer(0);
    ASSERT_from_bufferSendOutDrain_SIZE(numToDrain);
    ASSERT_from_bufferSendOutDrain(numToDrain - 1, this->m_buffers[numToDrain - 1]);
    ASSERT_EVENTS_BA_PartialDrainDone_SIZE(1);
    ASSERT_EVENTS_BA_PartialDrainDone(0, numToDrain);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, BufferAccumulator::OPCODE_BA_DRAINBUFFERS, 0, Fw::CmdResponse::OK);
    ASSERT_EQ(limit, this->lastOutstanding());

    // A second partial drain is accepted while the window is still full (REQ-BA-DL-015); it pends silently
    this->sendCmd_BA_DrainBuffers(0, 0, 1, BufferAccumulator_BlockMode::BLOCK);
    this->doDispatch();
    ASSERT_EVENTS_BA_StillDraining_SIZE(0);
    ASSERT_EVENTS_BA_DrainStalled_SIZE(0);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_from_bufferSendOutDrain_SIZE(numToDrain);

    // The next return opens a slot: the sixth buffer goes out and completes the second command on send
    this->returnBuffer(1);
    ASSERT_from_bufferSendOutDrain_SIZE(numBuffers);
    ASSERT_from_bufferSendOutDrain(numBuffers - 1, this->m_buffers[numBuffers - 1]);
    ASSERT_EVENTS_BA_PartialDrainDone_SIZE(2);
    ASSERT_EVENTS_BA_PartialDrainDone(1, 1u);
    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(1, BufferAccumulator::OPCODE_BA_DRAINBUFFERS, 0, Fw::CmdResponse::OK);

    // The remaining returns send nothing more: the queue is empty
    for (U32 i = 2; i < numBuffers; ++i) {
        this->returnBuffer(i);
        ASSERT_from_bufferSendOutDrain_SIZE(numBuffers);
    }
    ASSERT_from_bufferSendOutReturn_SIZE(numBuffers);
    ASSERT_EQ(0u, this->lastOutstanding());
    ASSERT_EQ(0u, this->lastQueued());
}

void BufferAccumulatorTester ::SetLimitDuringPartialDrain() {
    const U32 numToDrain = 4;
    this->sendCmd_BA_SetMode(0, 0, BufferAccumulator_OpState::ACCUMULATE);
    this->doDispatch();
    this->fill(numToDrain);

    // Limit 1: one buffer out, the command pends
    this->sendCmd_BA_DrainBuffers(0, 0, numToDrain, BufferAccumulator_BlockMode::BLOCK);
    this->doDispatch();
    ASSERT_from_bufferSendOutDrain_SIZE(1);
    ASSERT_CMD_RESPONSE_SIZE(1);

    // Rejected mid-drain too: the limit stays 1, nothing more goes out and the command still pends (REQ-BA-DL-005)
    this->sendCmd_BA_SetDrainLimit(0, 0, MAX_NUM_BUFFERS + 1);
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(1, BufferAccumulator::OPCODE_BA_SETDRAINLIMIT, 0, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_BA_InvalidDrainLimit_SIZE(1);
    ASSERT_EVENTS_BA_InvalidDrainLimit(0, MAX_NUM_BUFFERS + 1, MAX_NUM_BUFFERS);
    ASSERT_from_bufferSendOutDrain_SIZE(1);

    // Raising the limit mid-drain sends the rest and completes the command
    this->sendCmd_BA_SetDrainLimit(0, 0, numToDrain);
    this->doDispatch();
    ASSERT_from_bufferSendOutDrain_SIZE(numToDrain);
    ASSERT_CMD_RESPONSE_SIZE(4);
    ASSERT_CMD_RESPONSE(2, BufferAccumulator::OPCODE_BA_SETDRAINLIMIT, 0, Fw::CmdResponse::OK);
    ASSERT_CMD_RESPONSE(3, BufferAccumulator::OPCODE_BA_DRAINBUFFERS, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BA_DrainLimitSet_SIZE(1);
    ASSERT_EVENTS_BA_DrainLimitSet(0, numToDrain);
    ASSERT_TLM_BA_DrainLimit_SIZE(1);
    ASSERT_TLM_BA_DrainLimit(0, numToDrain);
    ASSERT_EVENTS_BA_PartialDrainDone_SIZE(1);
    ASSERT_EVENTS_BA_PartialDrainDone(0, numToDrain);
    ASSERT_EQ(numToDrain, this->lastOutstanding());
}

void BufferAccumulatorTester ::LowerLimitBelowOutstanding() {
    const U32 limit = 3;
    this->allocate(BufferAccumulator_OpState::ACCUMULATE, limit);
    this->fill(6);
    this->sendCmd_BA_SetMode(0, 0, BufferAccumulator_OpState::DRAIN);
    this->doDispatch();
    ASSERT_from_bufferSendOutDrain_SIZE(limit);

    this->sendCmd_BA_SetDrainLimit(0, 0, 1);
    this->doDispatch();
    ASSERT_CMD_RESPONSE(1, BufferAccumulator::OPCODE_BA_SETDRAINLIMIT, 0, Fw::CmdResponse::OK);
    ASSERT_from_bufferSendOutDrain_SIZE(limit);

    // Sends resume only once the outstanding count drops below the new limit
    this->returnBuffer(0);
    ASSERT_from_bufferSendOutDrain_SIZE(limit);
    ASSERT_EQ(2u, this->lastOutstanding());
    this->returnBuffer(1);
    ASSERT_from_bufferSendOutDrain_SIZE(limit);
    ASSERT_EQ(1u, this->lastOutstanding());
    this->returnBuffer(2);
    ASSERT_from_bufferSendOutDrain_SIZE(limit + 1);
    ASSERT_from_bufferSendOutDrain(limit, this->m_buffers[limit]);
    ASSERT_EQ(1u, this->lastOutstanding());
}

void BufferAccumulatorTester ::UnexpectedReturn() {
    this->m_buffers[0] = Fw::Buffer(this->m_data, sizeof(this->m_data), 0);
    this->m_buffers[1] = Fw::Buffer(this->m_data, sizeof(this->m_data), 1);

    // Nothing outstanding: the buffer is forwarded, the count stays 0, one warning
    this->returnBuffer(0);
    ASSERT_from_bufferSendOutReturn_SIZE(1);
    ASSERT_from_bufferSendOutReturn(0, this->m_buffers[0]);
    ASSERT_EVENTS_BA_UnexpectedReturn_SIZE(1);
    ASSERT_TLM_BA_NumOutstandingBuffers_SIZE(0);

    // Throttled: the next unexpected return is forwarded without a second warning
    this->returnBuffer(0);
    ASSERT_from_bufferSendOutReturn_SIZE(2);
    ASSERT_EVENTS_BA_UnexpectedReturn_SIZE(1);

    // An expected return re-arms the warning
    this->fill(1);
    ASSERT_from_bufferSendOutDrain_SIZE(1);
    ASSERT_EQ(1u, this->lastOutstanding());
    this->returnBuffer(0);
    ASSERT_from_bufferSendOutReturn_SIZE(3);
    ASSERT_EQ(0u, this->lastOutstanding());
    ASSERT_EVENTS_BA_UnexpectedReturn_SIZE(1);

    this->returnBuffer(1);
    ASSERT_from_bufferSendOutReturn_SIZE(4);
    ASSERT_from_bufferSendOutReturn(3, this->m_buffers[1]);
    ASSERT_EVENTS_BA_UnexpectedReturn_SIZE(2);
    ASSERT_EQ(0u, this->lastOutstanding());
}

void BufferAccumulatorTester ::AccumulateMidFlight() {
    const U32 limit = 2;
    this->allocate(BufferAccumulator_OpState::ACCUMULATE, limit);
    this->fill(4);
    this->sendCmd_BA_SetMode(0, 0, BufferAccumulator_OpState::DRAIN);
    this->doDispatch();
    ASSERT_from_bufferSendOutDrain_SIZE(limit);

    this->sendCmd_BA_SetMode(0, 0, BufferAccumulator_OpState::ACCUMULATE);
    this->doDispatch();
    ASSERT_CMD_RESPONSE(1, BufferAccumulator::OPCODE_BA_SETMODE, 0, Fw::CmdResponse::OK);

    // Returns are forwarded and counted, but nothing more is sent
    for (U32 i = 0; i < limit; ++i) {
        this->returnBuffer(i);
        ASSERT_from_bufferSendOutReturn_SIZE(i + 1);
        ASSERT_from_bufferSendOutReturn(i, this->m_buffers[i]);
        ASSERT_from_bufferSendOutDrain_SIZE(limit);
        ASSERT_EQ(limit - (i + 1), this->lastOutstanding());
    }
    ASSERT_EVENTS_BA_UnexpectedReturn_SIZE(0);
    ASSERT_EQ(2u, this->lastQueued());
}

void BufferAccumulatorTester ::StallAtLimit() {
    // DRAIN mode, limit 1: one buffer goes out and stays outstanding
    this->fill(1);
    ASSERT_from_bufferSendOutDrain_SIZE(1);
    this->sendCmd_BA_SetMode(0, 0, BufferAccumulator_OpState::ACCUMULATE);
    this->doDispatch();

    // The window is full, so the empty FIFO is not reported as a stall yet
    this->sendCmd_BA_DrainBuffers(0, 0, 1, BufferAccumulator_BlockMode::BLOCK);
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EVENTS_BA_DrainStalled_SIZE(0);

    // The return opens the window and finds nothing to send: stall reported, as before the change
    this->returnBuffer(0);
    ASSERT_EVENTS_BA_DrainStalled_SIZE(1);
    ASSERT_EVENTS_BA_DrainStalled(0, 0u, 1u);
    ASSERT_CMD_RESPONSE_SIZE(1);

    // A fill then completes the partial drain
    this->fill(1);
    ASSERT_from_bufferSendOutDrain_SIZE(2);
    ASSERT_EVENTS_BA_PartialDrainDone_SIZE(1);
    ASSERT_EVENTS_BA_PartialDrainDone(0, 1u);
    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(1, BufferAccumulator::OPCODE_BA_DRAINBUFFERS, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_BA_DrainStalled_SIZE(1);
}

void BufferAccumulatorTester ::FillWhileOutstanding() {
    // DRAIN mode, limit 3: each arrival goes straight out while the window has room (REQ-BA-DL-006)
    const U32 limit = 3;
    this->allocate(BufferAccumulator_OpState::DRAIN, limit);
    for (U32 i = 0; i < limit + 1; ++i) {
        this->m_buffers[i] = Fw::Buffer(this->m_data, sizeof(this->m_data), i);
        this->invoke_to_bufferSendInFill(0, this->m_buffers[i]);
        this->doDispatch();
        // The fourth arrival finds the window full and waits in the FIFO
        ASSERT_from_bufferSendOutDrain_SIZE(FW_MIN(i + 1, limit));
        ASSERT_EQ(FW_MIN(i + 1, limit), this->lastOutstanding());
    }
    ASSERT_EQ(1u, this->lastQueued());
    for (U32 i = 0; i < limit; ++i) {
        ASSERT_from_bufferSendOutDrain(i, this->m_buffers[i]);
    }

    // A return reopens the window and the queued buffer goes out
    this->returnBuffer(0);
    ASSERT_from_bufferSendOutDrain_SIZE(limit + 1);
    ASSERT_from_bufferSendOutDrain(limit, this->m_buffers[limit]);
    ASSERT_EQ(limit, this->lastOutstanding());
    ASSERT_EQ(0u, this->lastQueued());
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void BufferAccumulatorTester ::allocate(BufferAccumulator_OpState mode, U32 drainLimit) {
    this->component.allocateQueue(0, this->buffAccumMallocator, MAX_NUM_BUFFERS, mode, drainLimit);
    this->doAllocateQueue = true;
}

void BufferAccumulatorTester ::fill(U32 count) {
    for (U32 i = 0; i < count; ++i) {
        this->m_buffers[i] = Fw::Buffer(this->m_data, sizeof(this->m_data), i);
        this->invoke_to_bufferSendInFill(0, this->m_buffers[i]);
        this->doDispatch();
    }
}

void BufferAccumulatorTester ::returnBuffer(U32 i) {
    this->invoke_to_bufferSendInReturn(0, this->m_buffers[i]);
    this->doDispatch();
}

U32 BufferAccumulatorTester ::lastQueued() {
    const FwSizeType size = this->tlmHistory_BA_NumQueuedBuffers->size();
    EXPECT_GT(size, 0u);
    return (size > 0) ? static_cast<U32>(this->tlmHistory_BA_NumQueuedBuffers->at(static_cast<U32>(size - 1)).arg) : 0u;
}

U32 BufferAccumulatorTester ::lastOutstanding() {
    const FwSizeType size = this->tlmHistory_BA_NumOutstandingBuffers->size();
    EXPECT_GT(size, 0u);
    return (size > 0) ? static_cast<U32>(this->tlmHistory_BA_NumOutstandingBuffers->at(static_cast<U32>(size - 1)).arg)
                      : 0u;
}

}  // namespace DrainLimit

}  // namespace Svc
