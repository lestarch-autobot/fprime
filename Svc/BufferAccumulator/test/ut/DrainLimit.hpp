// ======================================================================
// \title  DrainLimit.hpp
// \author lestarch-autobot
// \brief  Test the drain limit (pipelined draining)
//
// \copyright
// Copyright (c) 2026 California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#ifndef Svc_DrainLimit_HPP
#define Svc_DrainLimit_HPP

#include "BufferAccumulatorTester.hpp"

namespace Svc {

namespace DrainLimit {

class BufferAccumulatorTester : public Svc::BufferAccumulatorTester {
  public:
    using Svc::BufferAccumulatorTester::BufferAccumulatorTester;

    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! allocateQueue without a limit defaults to 1; preamble reports limit and count (REQ-BA-DL-001/011/012/016)
    void DefaultLimit(void);

    //! allocateQueue with a limit > 1 pipelines that many buffers (REQ-BA-DL-002/006/007)
    void InitialLimit(void);

    //! allocateQueue with 0 or > maxNumBuffers asserts (REQ-BA-DL-017)
    void InitialLimitInvalid(void);

    //! BA_SetDrainLimit in range: OK, event, telemetry, opens slots; accepted in either mode
    //! (REQ-BA-DL-003/005/012/018)
    void SetLimitOK(void);

    //! BA_SetDrainLimit 0 or > maxNumBuffers: VALIDATION_ERROR, event, limit unchanged (REQ-BA-DL-004)
    void SetLimitInvalid(void);

    //! DRAIN mode keeps the window full, oldest first, as buffers return (REQ-BA-DL-006/007/008/011)
    void WindowedDrain(void);

    //! BA_DrainBuffers under a window completes when the target is sent (REQ-BA-DL-006/015)
    void WindowedPartialDrain(void);

    //! BA_SetDrainLimit during a partial drain is accepted and resumes sending (REQ-BA-DL-005)
    void SetLimitDuringPartialDrain(void);

    //! Lowering the limit below the outstanding count pauses sends until enough buffers return (REQ-BA-DL-007)
    void LowerLimitBelowOutstanding(void);

    //! A return with nothing outstanding is forwarded, not counted, and warned once (REQ-BA-DL-009/016)
    void UnexpectedReturn(void);

    //! ACCUMULATE with buffers in flight stops sends; returns are still processed (REQ-BA-DL-010)
    void AccumulateMidFlight(void);

    //! With the window full, BA_DrainBuffers does not report a stall until a return arrives (REQ-BA-DL-013)
    void StallAtLimit(void);

    //! A fill arriving in DRAIN mode is sent while the window has room
    void FillWhileOutstanding(void);

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Allocate the queue (deallocated by the base destructor) with the given mode and drain limit
    void allocate(BufferAccumulator_OpState mode, U32 drainLimit);

    //! Fill the first count buffers through bufferSendInFill
    void fill(U32 count);

    //! Return buffer i through bufferSendInReturn
    void returnBuffer(U32 i);

    //! The last reported BA_NumOutstandingBuffers value
    U32 lastQueued(void);
    U32 lastOutstanding(void);

    //! Backing store for the test buffers
    U8 m_data[10] = {};

    //! The test buffers
    Fw::Buffer m_buffers[MAX_NUM_BUFFERS];
};

}  // namespace DrainLimit

}  // namespace Svc

#endif
