// ======================================================================
// \title  BufferAccumulator.cpp
// \author bocchino
// \brief  BufferAccumulator implementation
//
// \copyright
// Copyright (C) 2017 California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include "Svc/BufferAccumulator/BufferAccumulator.hpp"

#include <limits>
#include "Fw/Types/BasicTypes.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Construction, initialization, and destruction
// ----------------------------------------------------------------------

BufferAccumulator ::BufferAccumulator(const char* const compName)
    : BufferAccumulatorComponentBase(compName),  //!< The component name
      m_mode(BufferAccumulator_OpState::ACCUMULATE),
      m_bufferMemory(nullptr),
      m_bufferQueue(),
      m_drainLimit(1u),
      m_numOutstanding(0u),
      m_unexpectedReturnReported(false),
      m_numWarnings(0u),
      m_numDrained(0u),
      m_numToDrain(0u),
      m_opCode(),
      m_cmdSeq(0u),
      m_allocatorId(0) {}

BufferAccumulator ::~BufferAccumulator() {}

// ----------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------

void BufferAccumulator ::allocateQueue(FwEnumStoreType identifier,
                                       Fw::MemAllocator& allocator,
                                       FwSizeType maxNumBuffers,               //!< The maximum number of buffers
                                       BufferAccumulator_OpState initialMode,  //!< The initial operating mode
                                       U32 initialDrainLimit  //!< The initial drain limit (1 .. maxNumBuffers)
) {
    this->m_allocatorId = identifier;
    // Overflow protection
    FW_ASSERT(maxNumBuffers > 0);
    // The drain limit is bounded by the queue depth so the consumer has a fixed number to size against
    FW_ASSERT(initialDrainLimit > 0, static_cast<FwAssertArgType>(initialDrainLimit));
    FW_ASSERT(static_cast<FwSizeType>(initialDrainLimit) <= maxNumBuffers,
              static_cast<FwAssertArgType>(initialDrainLimit), static_cast<FwAssertArgType>(maxNumBuffers));
    FW_ASSERT((std::numeric_limits<FwSizeType>::max() / maxNumBuffers) >= sizeof(Fw::Buffer));
    FwSizeType memSize = static_cast<FwSizeType>(sizeof(Fw::Buffer) * maxNumBuffers);
    bool recoverable = false;
    // A null or short allocation would be placement-new'd through by the queue below
    this->m_bufferMemory = static_cast<Fw::Buffer*>(allocator.checkedAllocate(identifier, memSize, recoverable));
    m_bufferQueue.init(this->m_bufferMemory, maxNumBuffers);
    this->m_mode = initialMode;
    this->m_drainLimit = initialDrainLimit;
}

void BufferAccumulator ::deallocateQueue(Fw::MemAllocator& allocator) {
    allocator.deallocate(static_cast<FwEnumStoreType>(this->m_allocatorId), this->m_bufferMemory);
}

void BufferAccumulator ::preamble() {
    this->tlmWrite_BA_DrainLimit(this->m_drainLimit);
    this->tlmWrite_BA_NumOutstandingBuffers(this->m_numOutstanding);
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined typed input ports
// ----------------------------------------------------------------------

void BufferAccumulator ::bufferSendInFill_handler(const FwIndexType portNum, Fw::Buffer& buffer) {
    const FwSizeType queuedBefore = this->m_bufferQueue.getSize();
    const U32 outstandingBefore = this->m_numOutstanding;
    const bool status = this->m_bufferQueue.enqueue(buffer);
    if (status) {
        if (this->m_numWarnings > 0) {
            this->log_ACTIVITY_HI_BA_BufferAccepted();
        }
        this->m_numWarnings = 0;
    } else {
        if (this->m_numWarnings == 0) {
            this->log_WARNING_HI_BA_QueueFull();
        }
        m_numWarnings++;
        // The buffer is dropped; ownership must go back to its sender or the pool is depleted
        this->bufferSendOutReturn_out(0, buffer);
    }
    // A fill never stalls a partial drain: the FIFO is non-empty here
    this->drain(false);
    this->reportCounts(queuedBefore, outstandingBefore);
}

void BufferAccumulator ::bufferSendInReturn_handler(const FwIndexType portNum, Fw::Buffer& buffer) {
    const FwSizeType queuedBefore = this->m_bufferQueue.getSize();
    const U32 outstandingBefore = this->m_numOutstanding;
    this->bufferSendOutReturn_out(0, buffer);
    if (this->m_numOutstanding > 0) {
        this->m_numOutstanding--;
        this->m_unexpectedReturnReported = false;
    } else if (!this->m_unexpectedReturnReported) {
        // A return with nothing outstanding (duplicate or foreign buffer); the count does not underflow
        this->log_WARNING_HI_BA_UnexpectedReturn();
        this->m_unexpectedReturnReported = true;
    }
    this->drain(true);
    this->reportCounts(queuedBefore, outstandingBefore);
}

void BufferAccumulator ::pingIn_handler(const FwIndexType portNum, U32 key) {
    this->pingOut_out(0, key);
}

// ----------------------------------------------------------------------
// Command handler implementations
// ----------------------------------------------------------------------

void BufferAccumulator ::BA_SetMode_cmdHandler(const FwOpcodeType opCode,
                                               const U32 cmdSeq,
                                               const BufferAccumulator_OpState& mode) {
    const FwSizeType queuedBefore = this->m_bufferQueue.getSize();
    const U32 outstandingBefore = this->m_numOutstanding;
    // cancel an in-progress partial drain
    if (this->m_numToDrain > 0) {
        // reset counters for partial buffer drain
        this->m_numToDrain = 0;
        this->m_numDrained = 0;
        // respond to the original command
        this->cmdResponse_out(this->m_opCode, this->m_cmdSeq, Fw::CmdResponse::OK);
    }

    this->m_mode = mode;
    // Sends stop in ACCUMULATE; buffers already outstanding still return and are counted
    this->drain(false);
    this->reportCounts(queuedBefore, outstandingBefore);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void BufferAccumulator ::BA_DrainBuffers_cmdHandler(const FwOpcodeType opCode,
                                                    const U32 cmdSeq,
                                                    U32 numToDrain,
                                                    const BufferAccumulator_BlockMode& blockMode) {
    const FwSizeType queuedBefore = this->m_bufferQueue.getSize();
    const U32 outstandingBefore = this->m_numOutstanding;
    if (this->m_numDrained < this->m_numToDrain) {
        this->log_WARNING_HI_BA_StillDraining(static_cast<U32>(this->m_numDrained),
                                              static_cast<U32>(this->m_numToDrain));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::BUSY);
        return;
    }

    if (this->m_mode == BufferAccumulator_OpState::DRAIN) {
        this->log_WARNING_HI_BA_AlreadyDraining();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }

    if (numToDrain == 0) {
        this->log_ACTIVITY_HI_BA_PartialDrainDone(0);
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
        return;
    }

    this->m_opCode = opCode;
    this->m_cmdSeq = cmdSeq;
    this->m_numDrained = 0;
    this->m_numToDrain = static_cast<FwSizeType>(numToDrain);

    if (blockMode == BufferAccumulator_BlockMode::NOBLOCK) {
        FwSizeType numBuffers = this->m_bufferQueue.getSize();

        if (numBuffers < static_cast<FwSizeType>(numToDrain)) {
            this->m_numToDrain = numBuffers;
            this->log_WARNING_LO_BA_NonBlockDrain(static_cast<U32>(this->m_numToDrain), numToDrain);
        }

        /* OK if there were 0 buffers queued, and we
         * end up setting numToDrain to 0
         */
        if (0 == this->m_numToDrain) {
            this->log_ACTIVITY_HI_BA_PartialDrainDone(0);
            this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
            return;
        }
    }

    // kick off the draining; the response is sent when the target has been sent
    this->drain(true);
    this->reportCounts(queuedBefore, outstandingBefore);
}

void BufferAccumulator ::BA_SetDrainLimit_cmdHandler(const FwOpcodeType opCode, const U32 cmdSeq, U32 maxOutstanding) {
    const FwSizeType maxAllowed = this->m_bufferQueue.getCapacity();
    if ((maxOutstanding == 0) || (static_cast<FwSizeType>(maxOutstanding) > maxAllowed)) {
        this->log_WARNING_HI_BA_InvalidDrainLimit(maxOutstanding, static_cast<U32>(maxAllowed));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    const FwSizeType queuedBefore = this->m_bufferQueue.getSize();
    const U32 outstandingBefore = this->m_numOutstanding;
    this->m_drainLimit = maxOutstanding;
    this->log_ACTIVITY_HI_BA_DrainLimitSet(maxOutstanding);
    this->tlmWrite_BA_DrainLimit(this->m_drainLimit);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    // A raised limit may open slots; a lowered one pauses sends until enough buffers return
    this->drain(false);
    this->reportCounts(queuedBefore, outstandingBefore);
}

// ----------------------------------------------------------------------
// Private helper methods
// ----------------------------------------------------------------------

bool BufferAccumulator ::isDraining() const {
    return (this->m_mode == BufferAccumulator_OpState::DRAIN) ||  // we are draining ALL buffers
           (this->m_numDrained < this->m_numToDrain);             // OR a partial drain is in progress
}

void BufferAccumulator ::drain(const bool reportStall) {
    const bool partialDrain = (this->m_numToDrain > 0);
    if (!this->isDraining() || (this->m_numOutstanding >= this->m_drainLimit)) {
        // Nothing to do, or the window is full: the stall check waits for a return (pre-change order)
        return;
    }
    if (partialDrain && reportStall && (this->m_bufferQueue.getSize() == 0)) {
        this->log_WARNING_HI_BA_DrainStalled(static_cast<U32>(this->m_numDrained),
                                             static_cast<U32>(this->m_numToDrain));
    }
    // Each iteration dequeues one buffer, so the pass is bounded by the FIFO depth
    const FwSizeType numQueued = this->m_bufferQueue.getSize();
    for (FwSizeType i = 0; i < numQueued; i++) {
        if ((this->m_numOutstanding >= this->m_drainLimit) ||
            (partialDrain && (this->m_numDrained >= this->m_numToDrain))) {
            break;
        }
        Fw::Buffer buffer;
        const bool status = this->m_bufferQueue.dequeue(buffer);
        FW_ASSERT(status);
        if (partialDrain) {
            this->m_numDrained++;
        }
        this->m_numOutstanding++;
        this->bufferSendOutDrain_out(0, buffer);
    }

    /* A partial drain completes once the target has been SENT, not returned */
    if (partialDrain && (this->m_numDrained == this->m_numToDrain)) {
        this->log_ACTIVITY_HI_BA_PartialDrainDone(static_cast<U32>(this->m_numDrained));
        // reset counters for partial buffer drain
        this->m_numToDrain = 0;
        this->m_numDrained = 0;
        this->cmdResponse_out(this->m_opCode, this->m_cmdSeq, Fw::CmdResponse::OK);
    }
}

void BufferAccumulator ::reportCounts(const FwSizeType queuedBefore, const U32 outstandingBefore) {
    if (this->m_bufferQueue.getSize() != queuedBefore) {
        this->tlmWrite_BA_NumQueuedBuffers(static_cast<U32>(this->m_bufferQueue.getSize()));
    }
    if (this->m_numOutstanding != outstandingBefore) {
        this->tlmWrite_BA_NumOutstandingBuffers(this->m_numOutstanding);
    }
}

}  // namespace Svc
