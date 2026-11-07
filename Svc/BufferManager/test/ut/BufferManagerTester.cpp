// ======================================================================
// \title  BufferManager.hpp
// \author tcanham
// \brief  cpp file for BufferManager test harness implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include "BufferManagerTester.hpp"
#include <Fw/Test/UnitTest.hpp>
#include <Fw/Types/MallocAllocator.hpp>
#include <cstdlib>
#include <limits>

#define INSTANCE 0
#define MAX_HISTORY_SIZE 100

// Bin buffer sizes/numbers

// Other constants

//! True if the memory ranges of two buffers do not overlap
static bool disjoint(const Fw::Buffer& a, const Fw::Buffer& b) {
    return (a.getData() + a.getCapacity() <= b.getData()) || (b.getData() + b.getCapacity() <= a.getData());
}

namespace Svc {

const FwEnumStoreType BufferManagerTester::MEM_ID;
const U16 BufferManagerTester::MGR_ID;
const FwSizeType BufferManagerTester::ANY_REQUEST_SIZE;
const Fw::Buffer::SizeType BufferManagerTester::BIN0_BUFFER_SIZE;
const U16 BufferManagerTester::BIN0_NUM_BUFFERS;
const Fw::Buffer::SizeType BufferManagerTester::BIN1_BUFFER_SIZE;
const U16 BufferManagerTester::BIN1_NUM_BUFFERS;
const Fw::Buffer::SizeType BufferManagerTester::BIN2_BUFFER_SIZE;
const U16 BufferManagerTester::BIN2_NUM_BUFFERS;
const U16 BufferManagerTester::TOTAL_NUM_BUFFERS;

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

BufferManagerTester ::BufferManagerTester()
    : BufferManagerGTestBase("Tester", MAX_HISTORY_SIZE), component("BufferManager") {
    this->initComponents();
    this->connectPorts();
}

BufferManagerTester ::~BufferManagerTester() {}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void BufferManagerTester ::contiguousLayout() {
    REQUIREMENT("FPRIME-BM-001");

    BufferManagerComponentImpl::BufferBins bins;
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[0].numBuffers = BIN0_NUM_BUFFERS;
    bins.bins[1].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[1].numBuffers = BIN1_NUM_BUFFERS;
    bins.bins[2].bufferSize = BIN2_BUFFER_SIZE;
    bins.bins[2].numBuffers = BIN2_NUM_BUFFERS;

    TestAllocator alloc;

    this->component.setup(MGR_ID, MEM_ID, alloc, bins);
    ASSERT_EQ(MEM_ID, this->component.m_memId);
    ASSERT_EQ(MGR_ID, this->component.m_mgrId);
    // check allocation state

    // Check that enough buffers were created
    ASSERT_EQ(BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS, this->component.m_numStructs);

    REQUIREMENT("FPRIME-BM-005");

    // check that enough memory was requested
    FwSizeType memSize = (BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS) *
                             sizeof(Svc::BufferManagerComponentImpl::AllocatedBuffer) +
                         (BIN0_NUM_BUFFERS * BIN0_BUFFER_SIZE + BIN1_NUM_BUFFERS * BIN1_BUFFER_SIZE +
                          BIN2_NUM_BUFFERS * BIN2_BUFFER_SIZE);
    ASSERT_EQ(memSize, alloc.getSize());

    // check that correct ID was requested
    ASSERT_EQ(MEM_ID, alloc.getId());

    // first buffer should point at location just past buffer structs
    U8* mem = reinterpret_cast<U8*>(alloc.getMem()) +
              this->component.m_numStructs * sizeof(Svc::BufferManagerComponentImpl::AllocatedBuffer);
    ;

    // check the buffer properties
    for (U16 entry = 0; entry < this->component.m_numStructs; entry++) {
        // check context ID
        ASSERT_EQ(this->component.m_buffers[entry].buff.getContext(), ((MGR_ID << 16) | entry));
        // check allocation state
        ASSERT_FALSE(this->component.m_buffers[entry].allocated);
        // check buffer sizes
        if (entry < BIN0_NUM_BUFFERS) {
            ASSERT_EQ(BIN0_BUFFER_SIZE, this->component.m_buffers[entry].size);
            ASSERT_EQ(mem, this->component.m_buffers[entry].memory);
            mem += BIN0_BUFFER_SIZE;
        } else if (entry < BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS) {
            ASSERT_EQ(BIN1_BUFFER_SIZE, this->component.m_buffers[entry].size);
            ASSERT_EQ(mem, this->component.m_buffers[entry].memory);
            mem += BIN1_BUFFER_SIZE;
        } else if (entry < BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS) {
            ASSERT_EQ(BIN2_BUFFER_SIZE, this->component.m_buffers[entry].size);
            ASSERT_EQ(mem, this->component.m_buffers[entry].memory);
            mem += BIN2_BUFFER_SIZE;
        } else {
            // just in case the logic is wrong
            ASSERT_TRUE(false);
        }
    }

    // memory location should be at end of allocated memory
    ASSERT_EQ(mem, reinterpret_cast<U8*>(alloc.getMem()) + alloc.getSize());

    this->component.cleanup();
    ASSERT_TRUE(this->component.m_cleaned);
    ASSERT_FALSE(this->component.m_setup);
}

void BufferManagerTester::oneBufferSize() {
    BufferManagerComponentImpl::BufferBins bins;
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[0].numBuffers = BIN1_NUM_BUFFERS;

    TestAllocator alloc;

    this->component.setup(MGR_ID, MEM_ID, alloc, bins);

    Fw::Buffer buffs[BIN1_NUM_BUFFERS];

    for (U16 b = 0; b < BIN1_NUM_BUFFERS; b++) {
        // Get the buffers
        buffs[b] = this->invoke_to_bufferGetCallee(0, BIN1_BUFFER_SIZE);
        // check allocation state
        ASSERT_TRUE(this->component.m_buffers[b].allocated);
        // check stats
        ASSERT_EQ(b + 1, this->component.m_currBuffs);
        // check stats
        ASSERT_EQ(b + 1, this->component.m_highWater);
    }

    // should send back empty buffer
    Fw::Buffer noBuff = this->invoke_to_bufferGetCallee(0, BIN1_BUFFER_SIZE);
    ASSERT_EQ(1, this->component.m_noBuffs);
    ASSERT_EVENTS_NoBuffsAvailable_SIZE(1);
    ASSERT_EVENTS_NoBuffsAvailable(0, BIN1_BUFFER_SIZE);

    // check telemetry
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_SIZE(5);
    ASSERT_TLM_TotalBuffs_SIZE(1);
    ASSERT_TLM_TotalBuffs(0, BIN1_NUM_BUFFERS);
    ASSERT_TLM_CurrBuffs_SIZE(1);
    ASSERT_TLM_CurrBuffs(0, BIN1_NUM_BUFFERS);
    ASSERT_TLM_HiBuffs_SIZE(1);
    ASSERT_TLM_HiBuffs(0, BIN1_NUM_BUFFERS);
    ASSERT_TLM_NoBuffs_SIZE(1);
    ASSERT_TLM_NoBuffs(0, 1);
    ASSERT_TLM_EmptyBuffs_SIZE(1);
    ASSERT_TLM_EmptyBuffs(0, 0);

    // clear histories
    this->clearHistory();

    REQUIREMENT("FPRIME-BM-006");

    // randomly return buffers
    bool returned[BIN1_NUM_BUFFERS] = {false};

    for (U16 b = 0; b < BIN1_NUM_BUFFERS; b++) {
        U16 entry;
        while (true) {
            entry = STest::Pick::lowerUpper(0, BIN1_NUM_BUFFERS - 1);
            if (not returned[entry]) {
                returned[entry] = true;
                break;
            }
        }
        // return the buffer
        printf("Returning buffer %d\n", entry);
        this->invoke_to_bufferSendIn(0, buffs[entry]);
        // check allocation state
        ASSERT_FALSE(this->component.m_buffers[entry].allocated);
        ASSERT_EQ(BIN1_NUM_BUFFERS - b - 1, this->component.m_currBuffs);
        ASSERT_EQ(BIN1_NUM_BUFFERS, this->component.m_highWater);
    }

    // should reject empty buffer
    this->invoke_to_bufferSendIn(0, noBuff);
    ASSERT_EQ(1, this->component.m_emptyBuffs);
    ASSERT_EVENTS_NullEmptyBuffer_SIZE(1);

    // check telemetry
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_SIZE(2);
    // No total buffs or no buffs since only on update
    ASSERT_TLM_CurrBuffs_SIZE(1);
    ASSERT_TLM_CurrBuffs(0, 0);
    ASSERT_TLM_NoBuffs_SIZE(0);
    ASSERT_TLM_EmptyBuffs_SIZE(1);
    ASSERT_TLM_EmptyBuffs(0, 1);

    // all buffers should be deallocated
    for (U16 b = 0; b < this->component.m_numStructs; b++) {
        ASSERT_FALSE(this->component.m_buffers[b].allocated);
    }

    // cleanup BufferManager memory
    this->component.cleanup();
}

void BufferManagerTester::multBuffSize() {
    BufferManagerComponentImpl::BufferBins bins;
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[0].numBuffers = BIN0_NUM_BUFFERS;
    bins.bins[1].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[1].numBuffers = BIN1_NUM_BUFFERS;
    bins.bins[2].bufferSize = BIN2_BUFFER_SIZE;
    bins.bins[2].numBuffers = BIN2_NUM_BUFFERS;

    TestAllocator alloc;

    this->component.setup(MGR_ID, MEM_ID, alloc, bins);

    Fw::Buffer buffs[BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS];

    REQUIREMENT("FPRIME-BM-002");

    // BufferManager should be able to provide the whole pool worth of buffers
    // for a requested size smaller than the smallest bin.

    for (U16 b = 0; b < BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS; b++) {
        // Get the buffers
        buffs[b] = this->invoke_to_bufferGetCallee(0, BIN0_BUFFER_SIZE);
        // check allocation state
        ASSERT_TRUE(this->component.m_buffers[b].allocated);
        // check stats
        ASSERT_EQ(b + 1, this->component.m_currBuffs);
        // check stats
        ASSERT_EQ(b + 1, this->component.m_highWater);
    }

    REQUIREMENT("FPRIME-BM-003");

    // should send back empty buffer
    Fw::Buffer noBuff = this->invoke_to_bufferGetCallee(0, BIN1_BUFFER_SIZE);
    ASSERT_EQ(1, this->component.m_noBuffs);
    ASSERT_EVENTS_NoBuffsAvailable_SIZE(1);
    ASSERT_EVENTS_NoBuffsAvailable(0, BIN1_BUFFER_SIZE);

    // check telemetry
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_SIZE(5);
    ASSERT_TLM_TotalBuffs_SIZE(1);
    ASSERT_TLM_TotalBuffs(0, BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS);
    ASSERT_TLM_CurrBuffs_SIZE(1);
    ASSERT_TLM_CurrBuffs(0, BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS);
    ASSERT_TLM_HiBuffs_SIZE(1);
    ASSERT_TLM_HiBuffs(0, BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS);
    ASSERT_TLM_NoBuffs_SIZE(1);
    ASSERT_TLM_NoBuffs(0, 1);
    ASSERT_TLM_EmptyBuffs_SIZE(1);
    ASSERT_TLM_EmptyBuffs(0, 0);

    // clear histories
    this->clearHistory();

    for (U16 b = 0; b < BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS; b++) {
        // return the buffer
        this->invoke_to_bufferSendIn(0, buffs[b]);
        // check allocation state
        ASSERT_FALSE(this->component.m_buffers[b].allocated);
        ASSERT_EQ(BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS - b - 1, this->component.m_currBuffs);
        ASSERT_EQ(BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS, this->component.m_highWater);
    }

    REQUIREMENT("FPRIME-BM-004");

    // should reject empty buffer
    this->invoke_to_bufferSendIn(0, noBuff);
    ASSERT_EQ(1, this->component.m_emptyBuffs);
    ASSERT_EVENTS_NullEmptyBuffer_SIZE(1);

    // check telemetry
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_SIZE(2);
    // No total buffs or no buffs since only on update
    ASSERT_TLM_CurrBuffs_SIZE(1);
    ASSERT_TLM_CurrBuffs(0, 0);
    ASSERT_TLM_NoBuffs_SIZE(0);
    ASSERT_TLM_EmptyBuffs_SIZE(1);
    ASSERT_TLM_EmptyBuffs(0, 1);

    // clear histories
    this->clearHistory();

    // all buffers should be deallocated
    for (U16 b = 0; b < this->component.m_numStructs; b++) {
        ASSERT_FALSE(this->component.m_buffers[b].allocated);
    }

    // clear histories
    this->clearEvents();

    // BufferManager should be able to provide the BIN1 and BIN2 worth of buffers
    // for a requested size just smaller than the BIN1 size

    for (U16 b = 0; b < BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS; b++) {
        // Get the buffers
        buffs[b] = this->invoke_to_bufferGetCallee(0, BIN1_BUFFER_SIZE);
        // check allocation state - should be allocating from bin 1
        ASSERT_TRUE(this->component.m_buffers[b + BIN0_NUM_BUFFERS].allocated);
        // check stats
        ASSERT_EQ(b + 1, this->component.m_currBuffs);
    }

    // should send back empty buffer
    noBuff = this->invoke_to_bufferGetCallee(0, BIN1_BUFFER_SIZE);
    ASSERT_EQ(2, this->component.m_noBuffs);
    ASSERT_EVENTS_NoBuffsAvailable_SIZE(1);
    ASSERT_EVENTS_NoBuffsAvailable(0, BIN1_BUFFER_SIZE);

    // check telemetry
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_SIZE(2);
    ASSERT_TLM_TotalBuffs_SIZE(0);
    ASSERT_TLM_CurrBuffs_SIZE(1);
    ASSERT_TLM_CurrBuffs(0, BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS);
    ASSERT_TLM_NoBuffs_SIZE(1);
    ASSERT_TLM_NoBuffs(0, 2);
    ASSERT_TLM_EmptyBuffs_SIZE(0);

    // clear histories
    this->clearHistory();

    for (U16 b = 0; b < BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS; b++) {
        // return the buffer
        this->invoke_to_bufferSendIn(0, buffs[b]);
        // check allocation state - should be freeing from bin 1
        ASSERT_FALSE(this->component.m_buffers[b + BIN0_NUM_BUFFERS].allocated);
        ASSERT_EQ(BIN1_NUM_BUFFERS + BIN2_NUM_BUFFERS - b - 1, this->component.m_currBuffs);
    }

    // should reject empty buffer
    this->invoke_to_bufferSendIn(0, noBuff);
    ASSERT_EQ(2, this->component.m_emptyBuffs);
    ASSERT_EVENTS_NullEmptyBuffer_SIZE(1);

    // check telemetry
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_SIZE(2);
    // No total buffs or no buffs since only on update
    ASSERT_TLM_CurrBuffs_SIZE(1);
    ASSERT_TLM_CurrBuffs(0, 0);
    ASSERT_TLM_NoBuffs_SIZE(0);
    ASSERT_TLM_EmptyBuffs_SIZE(1);
    ASSERT_TLM_EmptyBuffs(0, 2);

    // clear histories
    this->clearHistory();

    // all buffers should be deallocated
    for (U16 b = 0; b < this->component.m_numStructs; b++) {
        ASSERT_FALSE(this->component.m_buffers[b].allocated);
    }

    // BufferManager should be able to provide the BIN2 worth of buffers
    // for a requested size just smaller than the BIN2 size

    for (U16 b = 0; b < BIN2_NUM_BUFFERS; b++) {
        // Get the buffers
        buffs[b] = this->invoke_to_bufferGetCallee(0, BIN2_BUFFER_SIZE);
        // check allocation state - should be allocating from bin 1
        ASSERT_TRUE(this->component.m_buffers[b + BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS].allocated);
        // check stats
        ASSERT_EQ(b + 1, this->component.m_currBuffs);
    }

    // should send back empty buffer
    noBuff = this->invoke_to_bufferGetCallee(0, BIN2_BUFFER_SIZE);
    ASSERT_EQ(3, this->component.m_noBuffs);
    ASSERT_EVENTS_NoBuffsAvailable_SIZE(1);
    ASSERT_EVENTS_NoBuffsAvailable(0, BIN2_BUFFER_SIZE);

    // check telemetry
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_SIZE(2);
    ASSERT_TLM_TotalBuffs_SIZE(0);
    ASSERT_TLM_CurrBuffs_SIZE(1);
    ASSERT_TLM_CurrBuffs(0, BIN2_NUM_BUFFERS);
    ASSERT_TLM_NoBuffs_SIZE(1);
    ASSERT_TLM_NoBuffs(0, 3);
    ASSERT_TLM_EmptyBuffs_SIZE(0);

    // clear histories
    this->clearHistory();

    for (U16 b = 0; b < BIN2_NUM_BUFFERS; b++) {
        // return the buffer
        this->invoke_to_bufferSendIn(0, buffs[b]);
        // check allocation state - should be freeing from bin 1
        ASSERT_FALSE(this->component.m_buffers[b + BIN0_NUM_BUFFERS + BIN1_NUM_BUFFERS].allocated);
        ASSERT_EQ(BIN2_NUM_BUFFERS - b - 1, this->component.m_currBuffs);
    }

    // should reject empty buffer
    this->invoke_to_bufferSendIn(0, noBuff);
    ASSERT_EQ(3, this->component.m_emptyBuffs);
    ASSERT_EVENTS_NullEmptyBuffer_SIZE(1);

    // check telemetry
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_SIZE(2);
    // No total buffs or no buffs since only on update
    ASSERT_TLM_CurrBuffs_SIZE(1);
    ASSERT_TLM_CurrBuffs(0, 0);
    ASSERT_TLM_NoBuffs_SIZE(0);
    ASSERT_TLM_EmptyBuffs_SIZE(1);
    ASSERT_TLM_EmptyBuffs(0, 3);

    // all buffers should be deallocated
    for (U16 b = 0; b < this->component.m_numStructs; b++) {
        ASSERT_FALSE(this->component.m_buffers[b].allocated);
    }

    // cleanup BufferManager memory
    this->component.cleanup();
}

void BufferManagerTester::bufferSizeTrimmed() {
    REQUIREMENT("FPRIME-BM-007");

    BufferManagerComponentImpl::BufferBins bins;
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[0].numBuffers = 1;

    TestAllocator alloc;
    this->component.setup(MGR_ID, MEM_ID, alloc, bins);

    const Fw::Buffer::SizeType requestedSize = BIN1_BUFFER_SIZE - 1;
    Fw::Buffer buffer = this->invoke_to_bufferGetCallee(0, requestedSize);
    ASSERT_TRUE(buffer.isValid());
    ASSERT_EQ(requestedSize, buffer.getSize());

    this->invoke_to_bufferSendIn(0, buffer);
    ASSERT_FALSE(this->component.m_buffers[0].allocated);

    this->component.cleanup();
}

void BufferManagerTester::setupSizeOverflowAsserts() {
    // setup() adds up bufferSize + sizeof(AllocatedBuffer) per buffer, times numBuffers, across
    // bins, and hands the total to the allocator. Each step is now checked before it can wrap;
    // without the checks the total wrapped silently and the buffer structures were carved into
    // an allocation far smaller than the manager believed it had.
    const FwSizeType maxSize = std::numeric_limits<FwSizeType>::max();
    BufferManagerComponentImpl::BufferBins bins;
    TestAllocator alloc;

    // bufferSize + sizeof(AllocatedBuffer) wraps
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = maxSize;
    bins.bins[0].numBuffers = 1;
    ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, alloc, bins), "Assert: ");

    // per-buffer size times numBuffers wraps
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = maxSize / 2;
    bins.bins[0].numBuffers = 3;
    ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, alloc, bins), "Assert: ");

    // the running total across bins wraps
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = maxSize / 2;
    bins.bins[0].numBuffers = 1;
    bins.bins[1].bufferSize = maxSize / 2;
    bins.bins[1].numBuffers = 1;
    ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, alloc, bins), "Assert: ");
}

// ----------------------------------------------------------------------
// Test helpers
// ----------------------------------------------------------------------

void BufferManagerTester ::defaultBins(BufferManagerComponentImpl::BufferBins& bins) {
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[0].numBuffers = BIN0_NUM_BUFFERS;
    bins.bins[1].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[1].numBuffers = BIN1_NUM_BUFFERS;
    bins.bins[2].bufferSize = BIN2_BUFFER_SIZE;
    bins.bins[2].numBuffers = BIN2_NUM_BUFFERS;
}

Fw::Buffer BufferManagerTester ::getBuffer(FwSizeType size, FwSizeType expectedCapacity) {
    Fw::Buffer buffer = this->invoke_to_bufferGetCallee(0, size);
    EXPECT_NE(nullptr, buffer.getData());
    EXPECT_EQ(size, buffer.getSize());
    EXPECT_EQ(expectedCapacity, buffer.getCapacity());
    EXPECT_EQ(static_cast<U32>(MGR_ID), buffer.getContext() >> 16);
    return buffer;
}

void BufferManagerTester ::getNoBuffer(FwSizeType size) {
    this->clearEvents();
    Fw::Buffer buffer = this->invoke_to_bufferGetCallee(0, size);
    ASSERT_FALSE(buffer.isValid());
    ASSERT_EQ(nullptr, buffer.getData());
    ASSERT_EQ(0u, buffer.getSize());
    ASSERT_EVENTS_NoBuffsAvailable_SIZE(1);
    ASSERT_EVENTS_NoBuffsAvailable(0, size);
}

void BufferManagerTester ::returnBuffer(Fw::Buffer& buffer) {
    this->clearEvents();
    this->invoke_to_bufferSendIn(0, buffer);
    ASSERT_EVENTS_SIZE(0);
}

// ----------------------------------------------------------------------
// Directed tests observing the component only through its ports
// ----------------------------------------------------------------------

void BufferManagerTester ::setupBehavior() {
    REQUIREMENT("FPRIME-BM-001");
    REQUIREMENT("FPRIME-BM-005");

    BufferManagerComponentImpl::BufferBins bins;
    this->defaultBins(bins);
    TestAllocator alloc;
    this->component.setup(MGR_ID, MEM_ID, alloc, bins);

    // One allocation, with the configured identifier, large enough for every payload
    ASSERT_EQ(MEM_ID, alloc.getId());
    ASSERT_NE(nullptr, alloc.getMem());
    ASSERT_GE(alloc.getSize(), BIN0_NUM_BUFFERS * BIN0_BUFFER_SIZE + BIN1_NUM_BUFFERS * BIN1_BUFFER_SIZE +
                                   BIN2_NUM_BUFFERS * BIN2_BUFFER_SIZE);

    // Telemetry before any request
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_SIZE(5);
    ASSERT_TLM_TotalBuffs(0, TOTAL_NUM_BUFFERS);
    ASSERT_TLM_CurrBuffs(0, 0);
    ASSERT_TLM_HiBuffs(0, 0);
    ASSERT_TLM_NoBuffs(0, 0);
    ASSERT_TLM_EmptyBuffs(0, 0);

    // Every configured buffer can be obtained with the capacity of its bin
    const Fw::Buffer::SizeType sizes[] = {BIN0_BUFFER_SIZE, BIN1_BUFFER_SIZE, BIN2_BUFFER_SIZE};
    const U16 counts[] = {BIN0_NUM_BUFFERS, BIN1_NUM_BUFFERS, BIN2_NUM_BUFFERS};
    Fw::Buffer buffers[TOTAL_NUM_BUFFERS];
    U16 index = 0;
    for (U16 bin = 0; bin < FW_NUM_ARRAY_ELEMENTS(sizes); bin++) {
        for (U16 entry = 0; entry < counts[bin]; entry++) {
            buffers[index] = this->getBuffer(sizes[bin], sizes[bin]);
            index++;
        }
    }
    ASSERT_EVENTS_SIZE(0);

    // Buffers are distinct in identity and in memory
    for (U16 i = 0; i < TOTAL_NUM_BUFFERS; i++) {
        for (U16 j = 0; j < i; j++) {
            ASSERT_NE(buffers[i].getContext(), buffers[j].getContext());
            ASSERT_TRUE(disjoint(buffers[i], buffers[j]));
        }
    }

    // The pool is exhausted
    this->getNoBuffer(BIN0_BUFFER_SIZE);

    for (U16 i = 0; i < TOTAL_NUM_BUFFERS; i++) {
        this->returnBuffer(buffers[i]);
    }

    // Cleanup releases the single allocation through the same allocator
    this->component.cleanup();
    ASSERT_EQ(1u, alloc.getDeallocCount());
    ASSERT_EQ(alloc.getMem(), alloc.getDeallocPtr());
}

void BufferManagerTester ::bufferSizeSelection() {
    REQUIREMENT("FPRIME-BM-002");
    REQUIREMENT("FPRIME-BM-003");

    BufferManagerComponentImpl::BufferBins bins;
    this->defaultBins(bins);
    TestAllocator alloc;
    this->component.setup(MGR_ID, MEM_ID, alloc, bins);

    // A request between two bin sizes is served by the next larger bin
    Fw::Buffer between = this->getBuffer(BIN0_BUFFER_SIZE + 1, BIN1_BUFFER_SIZE);
    this->returnBuffer(between);

    // A request larger than every bin fails even though all buffers are free
    this->getNoBuffer(BIN2_BUFFER_SIZE + 1);

    // Exact bin sizes are served from their own bin
    Fw::Buffer exact0 = this->getBuffer(BIN0_BUFFER_SIZE, BIN0_BUFFER_SIZE);
    Fw::Buffer exact1 = this->getBuffer(BIN1_BUFFER_SIZE, BIN1_BUFFER_SIZE);
    Fw::Buffer exact2 = this->getBuffer(BIN2_BUFFER_SIZE, BIN2_BUFFER_SIZE);
    this->returnBuffer(exact0);
    this->returnBuffer(exact1);
    this->returnBuffer(exact2);

    // Once the smallest bin is exhausted a small request spills into the next bin
    Fw::Buffer small[BIN0_NUM_BUFFERS];
    for (U16 b = 0; b < BIN0_NUM_BUFFERS; b++) {
        small[b] = this->getBuffer(BIN0_BUFFER_SIZE, BIN0_BUFFER_SIZE);
    }
    Fw::Buffer spilled = this->getBuffer(BIN0_BUFFER_SIZE, BIN1_BUFFER_SIZE);

    // A freed small buffer is preferred again over the larger bins that are still free
    this->returnBuffer(small[0]);
    Fw::Buffer reused = this->getBuffer(BIN0_BUFFER_SIZE, BIN0_BUFFER_SIZE);
    this->returnBuffer(reused);
    for (U16 b = 1; b < BIN0_NUM_BUFFERS; b++) {
        this->returnBuffer(small[b]);
    }
    this->returnBuffer(spilled);

    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_SIZE(5);
    ASSERT_TLM_TotalBuffs(0, TOTAL_NUM_BUFFERS);
    ASSERT_TLM_CurrBuffs(0, 0);
    ASSERT_TLM_HiBuffs(0, 3);
    ASSERT_TLM_NoBuffs(0, 1);
    ASSERT_TLM_EmptyBuffs(0, 0);

    // A size-0 request is rejected with a warning and consumes nothing
    REQUIREMENT("FPRIME-BM-009");
    this->clearHistory();
    Fw::Buffer zero = this->invoke_to_bufferGetCallee(0, 0);
    ASSERT_FALSE(zero.isValid());
    ASSERT_EQ(nullptr, zero.getData());
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_ZeroSizeRequest_SIZE(1);
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_CurrBuffs_SIZE(0);  // CurrBuffs is update-on-change and was already reported as 0
    // and handing the empty result back is the ordinary empty-buffer warning
    this->clearHistory();
    this->invoke_to_bufferSendIn(0, zero);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_NullEmptyBuffer_SIZE(1);

    this->component.cleanup();
}

void BufferManagerTester ::eventThrottling() {
    REQUIREMENT("FPRIME-BM-003");
    REQUIREMENT("FPRIME-BM-004");

    BufferManagerComponentImpl::BufferBins bins;
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[0].numBuffers = 1;
    TestAllocator alloc;
    this->component.setup(MGR_ID, MEM_ID, alloc, bins);

    const U32 REQUESTS = BufferManagerTestState::EVENT_THROTTLE + 2;
    Fw::Buffer only = this->getBuffer(BIN0_BUFFER_SIZE, BIN0_BUFFER_SIZE);

    // Failed requests are reported until the event throttles
    for (U32 i = 0; i < REQUESTS; i++) {
        Fw::Buffer none = this->invoke_to_bufferGetCallee(0, BIN0_BUFFER_SIZE);
        ASSERT_FALSE(none.isValid());
    }
    ASSERT_EVENTS_NoBuffsAvailable_SIZE(BufferManagerTestState::EVENT_THROTTLE);
    for (U32 i = 0; i < BufferManagerTestState::EVENT_THROTTLE; i++) {
        ASSERT_EVENTS_NoBuffsAvailable(i, BIN0_BUFFER_SIZE);
    }

    // Empty returns are reported until the event throttles
    for (U32 i = 0; i < REQUESTS; i++) {
        Fw::Buffer empty;
        this->invoke_to_bufferSendIn(0, empty);
    }
    ASSERT_EVENTS_NullEmptyBuffer_SIZE(BufferManagerTestState::EVENT_THROTTLE);

    // Counters keep counting past the throttle
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_NoBuffs(0, REQUESTS);
    ASSERT_TLM_EmptyBuffs(0, REQUESTS);

    this->returnBuffer(only);

    // ZeroSizeRequest throttles the same way
    this->clearHistory();
    for (U32 i = 0; i < BufferManagerComponentImpl::EVENTID_ZEROSIZEREQUEST_THROTTLE + 2; i++) {
        Fw::Buffer zero = this->invoke_to_bufferGetCallee(0, 0);
        ASSERT_FALSE(zero.isValid());
    }
    ASSERT_EVENTS_ZeroSizeRequest_SIZE(BufferManagerComponentImpl::EVENTID_ZEROSIZEREQUEST_THROTTLE);
    this->component.cleanup();
}

void BufferManagerTester ::payloadIntegrity() {
    REQUIREMENT("FPRIME-BM-001");

    BufferManagerComponentImpl::BufferBins bins;
    this->defaultBins(bins);
    TestAllocator alloc;
    this->component.setup(MGR_ID, MEM_ID, alloc, bins);

    const Fw::Buffer::SizeType sizes[] = {BIN0_BUFFER_SIZE, BIN1_BUFFER_SIZE, BIN2_BUFFER_SIZE};
    const U16 counts[] = {BIN0_NUM_BUFFERS, BIN1_NUM_BUFFERS, BIN2_NUM_BUFFERS};
    Fw::Buffer buffers[TOTAL_NUM_BUFFERS];
    U16 index = 0;
    for (U16 bin = 0; bin < FW_NUM_ARRAY_ELEMENTS(sizes); bin++) {
        for (U16 entry = 0; entry < counts[bin]; entry++) {
            buffers[index] = this->getBuffer(sizes[bin], sizes[bin]);
            index++;
        }
    }

    // Fill every buffer with its own pattern over its full size
    for (U16 i = 0; i < TOTAL_NUM_BUFFERS; i++) {
        (void)memset(buffers[i].getData(), static_cast<int>(i + 1), buffers[i].getSize());
    }
    // No buffer's contents were disturbed by the writes to the others
    for (U16 i = 0; i < TOTAL_NUM_BUFFERS; i++) {
        for (FwSizeType k = 0; k < buffers[i].getSize(); k++) {
            ASSERT_EQ(static_cast<U8>(i + 1), buffers[i].getData()[k]);
        }
    }

    for (U16 i = 0; i < TOTAL_NUM_BUFFERS; i++) {
        this->returnBuffer(buffers[i]);
    }
    this->component.cleanup();
}

void BufferManagerTester ::lifecycle() {
    BufferManagerComponentImpl::BufferBins bins;
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[0].numBuffers = 1;

    // A single-buffer pool works, and cleanup() releases the pool exactly once however often it is called
    {
        TestAllocator alloc;
        this->component.setup(MGR_ID, MEM_ID, alloc, bins);
        Fw::Buffer only = this->getBuffer(BIN0_BUFFER_SIZE, BIN0_BUFFER_SIZE);
        this->getNoBuffer(BIN0_BUFFER_SIZE);
        this->returnBuffer(only);
        this->component.cleanup();
        this->component.cleanup();
        ASSERT_EQ(1u, alloc.getDeallocCount());
        ASSERT_EQ(alloc.getMem(), alloc.getDeallocPtr());
        // The ports reject use after cleanup
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferGetCallee(0, BIN0_BUFFER_SIZE), "Assert: ");
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, only), "Assert: ");

        // setup() after cleanup() yields a pool that cleanup() releases
        REQUIREMENT("FPRIME-BM-012");
        this->component.setup(MGR_ID, MEM_ID, alloc, bins);
        this->clearHistory();
        this->invoke_to_schedIn(0, 0);
        ASSERT_TLM_HiBuffs(0, 0);  // the high-water mark restarted with the new pool
        Fw::Buffer again = this->getBuffer(BIN0_BUFFER_SIZE, BIN0_BUFFER_SIZE);
        // ... and setting up over a live pool asserts
        REQUIREMENT("FPRIME-BM-011");
        ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, alloc, bins), "Assert: ");
        this->returnBuffer(again);
        this->component.cleanup();
        ASSERT_EQ(2u, alloc.getDeallocCount());
        ASSERT_EQ(alloc.getMem(), alloc.getDeallocPtr());
    }

    // A configuration with no buffers asserts
    REQUIREMENT("FPRIME-BM-010");
    {
        TestAllocator alloc;
        BufferManagerComponentImpl::BufferBins empty;
        memset(&empty, 0, sizeof(empty));
        ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, alloc, empty), "Assert: ");
    }

    // Destruction without an explicit cleanup() still releases the pool
    {
        TestAllocator alloc;
        {
            BufferManagerComponentImpl scoped("BufferManagerScoped");
            scoped.setup(MGR_ID, MEM_ID, alloc, bins);
            ASSERT_EQ(0u, alloc.getDeallocCount());
        }
        ASSERT_EQ(1u, alloc.getDeallocCount());
        ASSERT_EQ(alloc.getMem(), alloc.getDeallocPtr());
    }
}

void BufferManagerTester ::setupBinOrderAsserts() {
    REQUIREMENT("FPRIME-BM-008");

    TestAllocator alloc;
    BufferManagerComponentImpl::BufferBins bins;

    // Descending sizes are rejected
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[0].numBuffers = 1;
    bins.bins[1].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[1].numBuffers = 1;
    ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, alloc, bins), "Assert: ");

    // An empty bin between them does not hide the descending pair
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[0].numBuffers = 1;
    bins.bins[2].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[2].numBuffers = 1;
    ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, alloc, bins), "Assert: ");

    // Equal sizes and empty bins between ascending bins are accepted
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[0].numBuffers = 1;
    bins.bins[1].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[1].numBuffers = 1;
    bins.bins[3].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[3].numBuffers = 1;
    this->component.setup(MGR_ID, MEM_ID, alloc, bins);
    Fw::Buffer buffer = this->getBuffer(BIN1_BUFFER_SIZE, BIN1_BUFFER_SIZE);
    this->returnBuffer(buffer);
    this->component.cleanup();
}

void BufferManagerTester ::sendInAsserts() {
    BufferManagerComponentImpl::BufferBins bins;
    this->defaultBins(bins);
    TestAllocator alloc;
    this->component.setup(MGR_ID, MEM_ID, alloc, bins);

    Fw::Buffer held = this->getBuffer(BIN0_BUFFER_SIZE, BIN0_BUFFER_SIZE);
    const U32 id = held.getContext() & 0xFFFF;
    U8 foreign[BIN0_BUFFER_SIZE] = {};

    // Wrong manager ID
    {
        Fw::Buffer bad = held;
        bad.setContext(((static_cast<U32>(MGR_ID) + 1) << 16) | id);
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, bad), "Assert: ");
    }
    // Buffer ID beyond the pool
    {
        Fw::Buffer bad = held;
        bad.setContext((static_cast<U32>(MGR_ID) << 16) | TOTAL_NUM_BUFFERS);
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, bad), "Assert: ");
    }
    // Buffer ID of a buffer that was never handed out
    {
        Fw::Buffer bad(held.getData(), held.getSize(),
                       (static_cast<U32>(MGR_ID) << 16) | ((id + 1) % TOTAL_NUM_BUFFERS));
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, bad), "Assert: ");
    }
    // Pointer outside the buffer's own memory
    {
        Fw::Buffer bad(foreign, BIN0_BUFFER_SIZE, held.getContext());
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, bad), "Assert: ");
    }
    // Size larger than the buffer's allocation
    {
        Fw::Buffer bad(held.getData(), BIN0_BUFFER_SIZE + 1, held.getContext());
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, bad), "Assert: ");
    }
    // Returning the same buffer twice
    this->returnBuffer(held);
    ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, held), "Assert: ");

    this->component.cleanup();
}

void BufferManagerTester ::useBeforeSetupAsserts() {
    Fw::Buffer empty;
    ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferGetCallee(0, BIN0_BUFFER_SIZE), "Assert: ");
    ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, empty), "Assert: ");
    ASSERT_DEATH_IF_SUPPORTED(this->component.cleanup(), "Assert: ");
}

// ----------------------------------------------------------------------
// Rule-based testing setup
// ----------------------------------------------------------------------

void BufferManagerTester ::setupRules(const BufferManagerComponentImpl::BufferBins& bins) {
    this->shadow.shadow_bins = bins;
    this->Lifecycle__Setup__action();
}

void BufferManagerTester ::setupRulesOneBuffer() {
    BufferManagerComponentImpl::BufferBins bins;
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[0].numBuffers = 1;
    this->setupRules(bins);
}

void BufferManagerTester ::setupRulesOneBin() {
    BufferManagerComponentImpl::BufferBins bins;
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[0].numBuffers = BIN1_NUM_BUFFERS;
    this->setupRules(bins);
}

void BufferManagerTester ::setupRulesDefault() {
    BufferManagerComponentImpl::BufferBins bins;
    this->defaultBins(bins);
    this->setupRules(bins);
}

void BufferManagerTester ::setupRulesRandom() {
    BufferManagerComponentImpl::BufferBins bins;
    memset(&bins, 0, sizeof(bins));
    const U16 numBins = static_cast<U16>(STest::Pick::lowerUpper(1, 4));
    FwSizeType size = 0;
    for (U16 bin = 0; bin < numBins; bin++) {
        // Non-decreasing sizes, occasionally equal
        size += STest::Pick::lowerUpper((bin == 0) ? 1 : 0, 32);
        bins.bins[bin].bufferSize = size;
        bins.bins[bin].numBuffers = static_cast<U16>(STest::Pick::lowerUpper(1, 5));
    }
    this->setupRules(bins);
}

void BufferManagerTester ::teardownRules() {
    this->Lifecycle__Cleanup__action();
}

void BufferManagerTester ::ruleGet(FwSizeType size) {
    U16 bin = 0;
    ASSERT_TRUE(this->shadow.expectedBin(size, bin));

    Fw::Buffer buffer = this->invoke_to_bufferGetCallee(0, size);

    ASSERT_NE(nullptr, buffer.getData());
    ASSERT_EQ(size, buffer.getSize());
    ASSERT_EQ(this->shadow.shadow_binSize[bin], buffer.getCapacity());
    ASSERT_EQ(static_cast<U32>(MGR_ID), buffer.getContext() >> 16);
    ASSERT_FALSE(this->shadow.overlapsHeld(buffer));
    ASSERT_EVENTS_SIZE(0);

    // Stamp the payload so a later return can detect cross-talk between buffers
    const U8 pattern = static_cast<U8>(STest::Pick::lowerUpper(1, 255));
    (void)memset(buffer.getData(), pattern, buffer.getSize());
    this->shadow.recordGet(buffer, bin, size, pattern);
}

void BufferManagerTester ::ruleReturn(FwSizeType index, Fw::Buffer& buffer) {
    const BufferManagerTestState::HeldBuffer& held = this->shadow.shadow_held[index];
    for (FwSizeType k = 0; k < held.requested; k++) {
        ASSERT_EQ(held.pattern, held.buffer.getData()[k]);
    }

    this->invoke_to_bufferSendIn(0, buffer);

    ASSERT_EVENTS_SIZE(0);
    this->shadow.recordReturn(index);
}

// ----------------------------------------------------------------------
// Helper methods
// ----------------------------------------------------------------------

void BufferManagerTester ::connectPorts() {
    // bufferSendIn
    this->connect_to_bufferSendIn(0, this->component.get_bufferSendIn_InputPort(0));

    // bufferGetCallee
    this->connect_to_bufferGetCallee(0, this->component.get_bufferGetCallee_InputPort(0));

    // timeCaller
    this->component.set_timeCaller_OutputPort(0, this->get_from_timeCaller(0));

    // eventOut
    this->component.set_eventOut_OutputPort(0, this->get_from_eventOut(0));

    // textEventOut
    this->component.set_textEventOut_OutputPort(0, this->get_from_textEventOut(0));

    // tlmOut
    this->component.set_tlmOut_OutputPort(0, this->get_from_tlmOut(0));

    // schedIn
    this->connect_to_schedIn(0, this->component.get_schedIn_InputPort(0));
}

void BufferManagerTester ::initComponents() {
    this->init();
    this->component.init(INSTANCE);
}

void BufferManagerTester::textLogIn(const FwEventIdType id,          //!< The event ID
                                    const Fw::Time& timeTag,         //!< The time
                                    const Fw::LogSeverity severity,  //!< The severity
                                    const Fw::TextLogString& text    //!< The event string
) {
    TextLogEntry e = {id, timeTag, severity, text};

    printTextLogHistoryEntry(e, stdout);
}

}  // end namespace Svc
