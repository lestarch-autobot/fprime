// ======================================================================
// \title  BufferManagerTester.cpp
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
#include <algorithm>
#include <cstring>
#include <utility>
#include "Fw/Test/UnitTest.hpp"
#include "STest/Pick/Pick.hpp"

namespace {
//! Instance number given to the component under test
const FwEnumStoreType INSTANCE = 0;
//! Depth of the port, event and telemetry histories
const U32 MAX_HISTORY_SIZE = 100;

//! True if the memory ranges of two buffers do not overlap
bool disjoint(const Fw::Buffer& a, const Fw::Buffer& b) {
    return (a.getData() + a.getCapacity() <= b.getData()) || (b.getData() + b.getCapacity() <= a.getData());
}
}  // namespace

namespace Svc {

const FwEnumStoreType BufferManagerTester::MEM_ID;
const U16 BufferManagerTester::MGR_ID;
const FwSizeType BufferManagerTester::ANY_REQUEST_SIZE;
const U32 BufferManagerTester::NUM_TLM_CHANNELS;
constexpr const char* BufferManagerTester::UUT_ASSERT;
const FwSizeType BufferManagerTester::DEFAULT_ALIGNMENT;
const FwSizeType BufferManagerTester::ABOVE_DEFAULT_ALIGNMENT;
const FwSizeType BufferManagerTester::MIN_ALIGNMENT;
const Fw::Buffer::SizeType BufferManagerTester::BIN0_BUFFER_SIZE;
const U16 BufferManagerTester::BIN0_NUM_BUFFERS;
const Fw::Buffer::SizeType BufferManagerTester::BIN1_BUFFER_SIZE;
const U16 BufferManagerTester::BIN1_NUM_BUFFERS;
const Fw::Buffer::SizeType BufferManagerTester::BIN2_BUFFER_SIZE;
const U16 BufferManagerTester::BIN2_NUM_BUFFERS;
const U16 BufferManagerTester::NUM_BINS;
constexpr Fw::Buffer::SizeType BufferManagerTester::BIN_SIZES[];
constexpr U16 BufferManagerTester::BIN_COUNTS[];
const U16 BufferManagerTester::TOTAL_NUM_BUFFERS;

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

BufferManagerTester ::BufferManagerTester()
    : BufferManagerGTestBase("Tester", MAX_HISTORY_SIZE),
      component("BufferManager"),
      m_poolAlignment(DEFAULT_ALIGNMENT) {
    this->initComponents();
    this->connectPorts();
}

BufferManagerTester ::~BufferManagerTester() {}

// ----------------------------------------------------------------------
// Directed tests
// ----------------------------------------------------------------------

void BufferManagerTester ::setupBehavior() {
    REQUIREMENT("FPRIME-BM-001");
    REQUIREMENT("FPRIME-BM-005");

    // The pool is requested from the supplied allocator under the configured identifier (setupPool checks the
    // allocation contract; given no alignment, setup() uses the allocator's own default)
    this->setupPool(defaultBins());

    // Telemetry before any request
    this->checkTelemetry(NUM_TLM_CHANNELS);
    ASSERT_TLM_TotalBuffs(0, TOTAL_NUM_BUFFERS);
    ASSERT_TLM_CurrBuffs(0, 0);
    ASSERT_TLM_HiBuffs(0, 0);
    ASSERT_TLM_NoBuffs(0, 0);
    ASSERT_TLM_EmptyBuffs(0, 0);

    // Every configured buffer can be obtained with the capacity of its bin
    Fw::Buffer buffers[TOTAL_NUM_BUFFERS];
    this->getAllBuffers(buffers);

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

    // Cleanup releases everything that was allocated, through the same allocator
    this->cleanupPool();
}

void BufferManagerTester ::setupAlignment() {
    REQUIREMENT("FPRIME-BM-005");
    COMMENT(
        "The alignment given to setup() reaches the allocator and the buffers starting an allocation have it (SDD "
        "4.2)");

    const FwSizeType alignments[] = {MIN_ALIGNMENT,  // below anything setup() places in the pool: the request is raised
                                     DEFAULT_ALIGNMENT,             // the default, given explicitly
                                     ABOVE_DEFAULT_ALIGNMENT,       // stricter than the default: passed on as is
                                     4 * ABOVE_DEFAULT_ALIGNMENT};  // well above: still passed on as is
    for (FwSizeType alignment : alignments) {
        this->setupPool(defaultBins(), alignment);

        // Payloads start at the beginning of the memory the allocator hands back: at least one buffer of the smallest
        // bin starts an allocation, and every such buffer has the alignment given, requested of the allocator as a
        // power of two no looser than it (getBuffer checks that; whichever order the bin hands them out)
        Fw::Buffer smallest[BIN0_NUM_BUFFERS];
        U16 atBase = 0;
        for (U16 b = 0; b < BIN0_NUM_BUFFERS; b++) {
            smallest[b] = this->getBuffer(BIN0_BUFFER_SIZE, BIN0_BUFFER_SIZE);
            if (this->m_allocator.startsLiveAllocation(smallest[b].getData())) {
                atBase++;
            }
        }
        ASSERT_GE(atBase, 1);
        for (U16 b = 0; b < BIN0_NUM_BUFFERS; b++) {
            this->returnBuffer(smallest[b]);
        }
        this->cleanupPool();
    }
}

void BufferManagerTester ::singleBin() {
    REQUIREMENT("FPRIME-BM-006");

    this->setupPool(oneBin(BIN1_BUFFER_SIZE, BIN1_NUM_BUFFERS));

    // Every buffer of the bin can be obtained, then the bin is exhausted
    Fw::Buffer buffers[BIN1_NUM_BUFFERS];
    for (U16 b = 0; b < BIN1_NUM_BUFFERS; b++) {
        buffers[b] = this->getBuffer(BIN1_BUFFER_SIZE, BIN1_BUFFER_SIZE);
    }
    this->getNoBuffer(BIN1_BUFFER_SIZE);
    this->checkTelemetry(NUM_TLM_CHANNELS);
    ASSERT_TLM_TotalBuffs(0, BIN1_NUM_BUFFERS);
    ASSERT_TLM_CurrBuffs(0, BIN1_NUM_BUFFERS);
    ASSERT_TLM_HiBuffs(0, BIN1_NUM_BUFFERS);
    ASSERT_TLM_NoBuffs(0, 1);
    ASSERT_TLM_EmptyBuffs(0, 0);

    // Buffers are accepted back in any order: Fisher-Yates shuffle of the return order
    U16 order[BIN1_NUM_BUFFERS];
    for (U16 b = 0; b < BIN1_NUM_BUFFERS; b++) {
        order[b] = b;
    }
    for (U16 remaining = BIN1_NUM_BUFFERS; remaining > 1; remaining--) {
        const U16 pick = static_cast<U16>(STest::Pick::lowerUpper(0, remaining - 1));
        std::swap(order[remaining - 1], order[pick]);
    }
    for (U16 b = 0; b < BIN1_NUM_BUFFERS; b++) {
        this->returnBuffer(buffers[order[b]]);
    }
    // An empty buffer (what a failed request returns) is rejected with a warning
    Fw::Buffer empty;
    this->returnEmptyBuffer(empty);
    // Update on change: only CurrBuffs and EmptyBuffs moved
    this->checkTelemetry(2);
    ASSERT_TLM_CurrBuffs(0, 0);
    ASSERT_TLM_EmptyBuffs(0, 1);

    // Every buffer is available again
    for (U16 b = 0; b < BIN1_NUM_BUFFERS; b++) {
        buffers[b] = this->getBuffer(BIN1_BUFFER_SIZE, BIN1_BUFFER_SIZE);
    }
    for (U16 b = 0; b < BIN1_NUM_BUFFERS; b++) {
        this->returnBuffer(buffers[b]);
    }
    this->cleanupPool();
}

void BufferManagerTester ::multipleBins() {
    REQUIREMENT("FPRIME-BM-002");
    REQUIREMENT("FPRIME-BM-003");
    REQUIREMENT("FPRIME-BM-004");

    this->setupPool(defaultBins());

    // Requests of one bin's size drain that bin and then every larger bin, smallest first; the bins
    // below it cannot serve them
    for (U16 first = 0; first < NUM_BINS; first++) {
        Fw::Buffer buffers[TOTAL_NUM_BUFFERS];
        U16 obtained = 0;
        for (U16 bin = first; bin < NUM_BINS; bin++) {
            for (U16 entry = 0; entry < BIN_COUNTS[bin]; entry++) {
                buffers[obtained] = this->getBuffer(BIN_SIZES[first], BIN_SIZES[bin]);
                obtained++;
            }
        }
        this->getNoBuffer(BIN_SIZES[first]);
        // First pass writes every channel; later passes only CurrBuffs and NoBuffs change
        this->checkTelemetry((first == 0) ? NUM_TLM_CHANNELS : 2);
        ASSERT_TLM_CurrBuffs(0, obtained);
        ASSERT_TLM_NoBuffs(0, first + 1);
        if (first == 0) {
            ASSERT_TLM_TotalBuffs(0, TOTAL_NUM_BUFFERS);
            ASSERT_TLM_HiBuffs(0, TOTAL_NUM_BUFFERS);
        }

        for (U16 i = 0; i < obtained; i++) {
            this->returnBuffer(buffers[i]);
        }
        Fw::Buffer empty;
        this->returnEmptyBuffer(empty);
        this->checkTelemetry(2);
        ASSERT_TLM_CurrBuffs(0, 0);
        ASSERT_TLM_EmptyBuffs(0, first + 1);
    }
    this->cleanupPool();
}

void BufferManagerTester ::trimmedReturn() {
    REQUIREMENT("FPRIME-BM-007");

    this->setupPool(oneBin(BIN1_BUFFER_SIZE, 1));

    // The size is trimmed to the request while the capacity stays the bin's
    Fw::Buffer buffer = this->getBuffer(BIN1_BUFFER_SIZE - 1, BIN1_BUFFER_SIZE);
    this->returnBuffer(buffer);
    // A trimmed buffer goes back to its slot: the only buffer is available again at full size
    buffer = this->getBuffer(BIN1_BUFFER_SIZE, BIN1_BUFFER_SIZE);
    this->returnBuffer(buffer);
    this->cleanupPool();
}

void BufferManagerTester ::bufferSizeSelection() {
    REQUIREMENT("FPRIME-BM-002");
    REQUIREMENT("FPRIME-BM-003");

    this->setupPool(defaultBins());

    // A request between two bin sizes is served by the next larger bin
    Fw::Buffer between = this->getBuffer(BIN0_BUFFER_SIZE + 1, BIN1_BUFFER_SIZE);
    this->returnBuffer(between);

    // A request larger than every bin fails even though all buffers are free
    this->getNoBuffer(BIN2_BUFFER_SIZE + 1);

    // Exact bin sizes are served from their own bin
    Fw::Buffer exact[NUM_BINS];
    for (U16 bin = 0; bin < NUM_BINS; bin++) {
        exact[bin] = this->getBuffer(BIN_SIZES[bin], BIN_SIZES[bin]);
    }
    for (U16 bin = 0; bin < NUM_BINS; bin++) {
        this->returnBuffer(exact[bin]);
    }

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

    // The most buffers out at once: one per bin, or the small bin plus the one that spilled
    const U32 peak = std::max<U32>(NUM_BINS, BIN0_NUM_BUFFERS + 1);
    this->checkTelemetry(NUM_TLM_CHANNELS);
    ASSERT_TLM_TotalBuffs(0, TOTAL_NUM_BUFFERS);
    ASSERT_TLM_CurrBuffs(0, 0);
    ASSERT_TLM_HiBuffs(0, peak);
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
    // Update on change: only NoBuffs moved (1 -> 2), nothing was allocated
    this->checkTelemetry(1);
    ASSERT_TLM_NoBuffs(0, 2);
    // and handing the empty result back is the ordinary empty-buffer warning
    this->returnEmptyBuffer(zero);

    this->cleanupPool();
}

void BufferManagerTester ::eventThrottling() {
    REQUIREMENT("FPRIME-BM-003");
    REQUIREMENT("FPRIME-BM-004");
    REQUIREMENT("FPRIME-BM-009");

    this->setupPool(oneBin(BIN0_BUFFER_SIZE, 1));

    const U32 requests = BufferManagerTestState::EVENT_THROTTLE + 2;
    Fw::Buffer only = this->getBuffer(BIN0_BUFFER_SIZE, BIN0_BUFFER_SIZE);

    // Failed requests are reported until the event throttles
    for (U32 i = 0; i < requests; i++) {
        Fw::Buffer none = this->invoke_to_bufferGetCallee(0, BIN0_BUFFER_SIZE);
        ASSERT_FALSE(none.isValid());
    }
    ASSERT_EVENTS_NoBuffsAvailable_SIZE(BufferManagerTestState::EVENT_THROTTLE);
    for (U32 i = 0; i < BufferManagerTestState::EVENT_THROTTLE; i++) {
        ASSERT_EVENTS_NoBuffsAvailable(i, BIN0_BUFFER_SIZE);
    }

    // Empty returns are reported until the event throttles
    for (U32 i = 0; i < requests; i++) {
        Fw::Buffer empty;
        this->invoke_to_bufferSendIn(0, empty);
    }
    ASSERT_EVENTS_NullEmptyBuffer_SIZE(BufferManagerTestState::EVENT_THROTTLE);

    // Counters keep counting past the throttle
    this->checkTelemetry(NUM_TLM_CHANNELS);
    ASSERT_TLM_NoBuffs(0, requests);
    ASSERT_TLM_EmptyBuffs(0, requests);

    this->returnBuffer(only);

    // ZeroSizeRequest throttles the same way, and each rejection still counts in NoBuffs
    this->clearHistory();
    for (U32 i = 0; i < requests; i++) {
        Fw::Buffer zero = this->invoke_to_bufferGetCallee(0, 0);
        ASSERT_FALSE(zero.isValid());
    }
    ASSERT_EVENTS_ZeroSizeRequest_SIZE(BufferManagerTestState::EVENT_THROTTLE);
    this->checkTelemetry(2);  // CurrBuffs (the return) and NoBuffs moved
    ASSERT_TLM_CurrBuffs(0, 0);
    ASSERT_TLM_NoBuffs(0, 2 * requests);
    this->cleanupPool();
}

void BufferManagerTester ::payloadIntegrity() {
    COMMENT("Writes over one buffer's full size never disturb another buffer");

    this->setupPool(defaultBins());

    Fw::Buffer buffers[TOTAL_NUM_BUFFERS];
    this->getAllBuffers(buffers);

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
    this->cleanupPool();
}

void BufferManagerTester ::lifecycle() {
    const BufferManagerComponentImpl::BufferBins bins = oneBin(BIN0_BUFFER_SIZE, 1);

    // A single-buffer pool works, and cleanup() releases the pool exactly once however often it is called
    {
        this->setupPool(bins);
        Fw::Buffer only = this->getBuffer(BIN0_BUFFER_SIZE, BIN0_BUFFER_SIZE);
        this->getNoBuffer(BIN0_BUFFER_SIZE);
        this->returnBuffer(only);
        this->cleanupPool();
        this->component.cleanup();  // a second call is a no-op
        this->checkAllReleased(this->m_allocator);
        // The ports reject use after cleanup
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferGetCallee(0, BIN0_BUFFER_SIZE), UUT_ASSERT);
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, only), UUT_ASSERT);

        // setup() after cleanup() yields a pool that cleanup() releases
        REQUIREMENT("FPRIME-BM-012");
        TestAllocator other;
        const U32 ownDeallocs = this->m_allocator.getDeallocCount();
        this->component.setup(MGR_ID, MEM_ID, other, bins);
        ASSERT_GE(other.getLiveCount(), 1u);
        this->checkTelemetry(NUM_TLM_CHANNELS);
        ASSERT_TLM_HiBuffs(0, 0);  // the high-water mark restarted with the new pool
        Fw::Buffer again = this->getBuffer(BIN0_BUFFER_SIZE, BIN0_BUFFER_SIZE);
        // ... and setting up over a live pool asserts
        REQUIREMENT("FPRIME-BM-011");
        this->expectSetupDeath(bins);
        this->returnBuffer(again);
        this->component.cleanup();
        // the second pool goes back to the allocator it came from, all of it
        ASSERT_EQ(ownDeallocs, this->m_allocator.getDeallocCount());
        this->checkAllReleased(other);
    }

    // A configuration with no buffers, or with zero-sized buffers, asserts
    REQUIREMENT("FPRIME-BM-010");
    this->Lifecycle__EmptyBinsAsserts__action();

    // Destruction without an explicit cleanup() still releases the pool
    {
        TestAllocator alloc;
        {
            BufferManagerComponentImpl scoped("BufferManagerScoped");
            scoped.setup(MGR_ID, MEM_ID, alloc, bins);
            ASSERT_GE(alloc.getLiveCount(), 1u);
            ASSERT_EQ(0u, alloc.getDeallocCount());
        }
        this->checkAllReleased(alloc);
    }
}

void BufferManagerTester ::setupSizeOverflowAsserts() {
    COMMENT("setup() checks each step of its pool-size arithmetic before it can wrap");
    this->Lifecycle__SizeOverflowAsserts__action();
}

void BufferManagerTester ::setupBinOrderAsserts() {
    REQUIREMENT("FPRIME-BM-008");
    this->Lifecycle__BinOrderAsserts__action();

    // Equal sizes and empty bins between ascending bins are accepted
    this->setupPool(equalAndGappedBins());
    Fw::Buffer buffer = this->getBuffer(BIN1_BUFFER_SIZE, BIN1_BUFFER_SIZE);
    this->returnBuffer(buffer);
    this->cleanupPool();
}

void BufferManagerTester ::setupAlignmentAsserts() {
    COMMENT("setup() rejects an alignment that is zero or not a power of two, and memory not aligned as asked");
    this->Lifecycle__AlignmentAsserts__action();
}

void BufferManagerTester ::sendInAsserts() {
    COMMENT("bufferSendIn asserts on a buffer that is not one it handed out (SDD 3.6.2)");
    this->setupRulesDefault();
    this->BufferGet__Ok__action();
    this->BufferReturn__Asserts__action();
    this->teardownRules();
}

void BufferManagerTester ::useBeforeSetupAsserts() {
    COMMENT("The ports and cleanup() assert before setup()");
    this->Lifecycle__NotSetupAsserts__action();
}

// ----------------------------------------------------------------------
// Rule-based testing setup
// ----------------------------------------------------------------------

void BufferManagerTester ::setupRules(const BufferManagerComponentImpl::BufferBins& bins, FwSizeType alignment) {
    this->shadow.stageBins(bins, alignment);
    this->Lifecycle__Setup__action();
}

void BufferManagerTester ::setupRulesOneBuffer() {
    this->setupRules(oneBin(BIN0_BUFFER_SIZE, 1));
}

void BufferManagerTester ::setupRulesOneBin() {
    this->setupRules(oneBin(BIN1_BUFFER_SIZE, BIN1_NUM_BUFFERS));
}

void BufferManagerTester ::setupRulesDefault() {
    this->setupRules(defaultBins());
}

void BufferManagerTester ::setupRulesEqualAndGapped() {
    this->setupRules(equalAndGappedBins());
}

void BufferManagerTester ::setupRulesRandom() {
    // Small tables (at most 4 bins of at most 5 buffers, sizes stepping by at most 32) so that exhaustion,
    // spill-over and between-bin sizes all occur often within a scenario
    BufferManagerComponentImpl::BufferBins bins = {};
    const U16 numBins = static_cast<U16>(STest::Pick::lowerUpper(1, 4));
    FwSizeType size = 0;
    for (U16 bin = 0; bin < numBins; bin++) {
        // Non-decreasing sizes, occasionally equal
        size += STest::Pick::lowerUpper((bin == 0) ? 1 : 0, 32);
        bins.bins[bin].bufferSize = size;
        bins.bins[bin].numBuffers = static_cast<U16>(STest::Pick::lowerUpper(1, 5));
    }
    // Any power of two from a single byte up to a page
    const FwSizeType alignment = static_cast<FwSizeType>(1) << STest::Pick::lowerUpper(0, 12);
    this->setupRules(bins, alignment);
}

void BufferManagerTester ::teardownRules() {
    // A random scenario may end with the pool already cleaned up by a lifecycle rule
    if (this->shadow.isSetup) {
        this->Lifecycle__Cleanup__action();
    }
}

// ----------------------------------------------------------------------
// Bin table builders
// ----------------------------------------------------------------------

BufferManagerComponentImpl::BufferBins BufferManagerTester ::defaultBins() {
    BufferManagerComponentImpl::BufferBins bins = {};
    for (U16 bin = 0; bin < NUM_BINS; bin++) {
        bins.bins[bin].bufferSize = BIN_SIZES[bin];
        bins.bins[bin].numBuffers = BIN_COUNTS[bin];
    }
    return bins;
}

BufferManagerComponentImpl::BufferBins BufferManagerTester ::oneBin(Fw::Buffer::SizeType size, U16 count) {
    BufferManagerComponentImpl::BufferBins bins = {};
    bins.bins[0].bufferSize = size;
    bins.bins[0].numBuffers = count;
    return bins;
}

BufferManagerComponentImpl::BufferBins BufferManagerTester ::twoBins(Fw::Buffer::SizeType size0,
                                                                     U16 count0,
                                                                     U16 index1,
                                                                     Fw::Buffer::SizeType size1,
                                                                     U16 count1) {
    BufferManagerComponentImpl::BufferBins bins = oneBin(size0, count0);
    bins.bins[index1].bufferSize = size1;
    bins.bins[index1].numBuffers = count1;
    return bins;
}

BufferManagerComponentImpl::BufferBins BufferManagerTester ::equalAndGappedBins() {
    BufferManagerComponentImpl::BufferBins bins = twoBins(BIN0_BUFFER_SIZE, 1, 1, BIN0_BUFFER_SIZE, 1);
    bins.bins[3].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[3].numBuffers = 1;
    return bins;
}

// ----------------------------------------------------------------------
// Port helpers
// ----------------------------------------------------------------------

Fw::Buffer BufferManagerTester ::getBuffer(FwSizeType size, FwSizeType expectedCapacity) {
    this->clearHistory();
    Fw::Buffer buffer = this->invoke_to_bufferGetCallee(0, size);
    EXPECT_NE(nullptr, buffer.getData());
    EXPECT_EQ(size, buffer.getSize());
    EXPECT_EQ(expectedCapacity, buffer.getCapacity());
    EXPECT_EQ(static_cast<U32>(MGR_ID), BufferManagerTestState::managerIdOf(buffer));
    this->checkAllocationAlignment(buffer);
    ASSERT_EVENTS_SIZE(0);
    return buffer;
}

void BufferManagerTester ::getAllBuffers(Fw::Buffer (&buffers)[TOTAL_NUM_BUFFERS]) {
    U16 index = 0;
    for (U16 bin = 0; bin < NUM_BINS; bin++) {
        for (U16 entry = 0; entry < BIN_COUNTS[bin]; entry++) {
            buffers[index] = this->getBuffer(BIN_SIZES[bin], BIN_SIZES[bin]);
            index++;
        }
    }
}

void BufferManagerTester ::getNoBuffer(FwSizeType size) {
    this->clearHistory();
    Fw::Buffer buffer = this->invoke_to_bufferGetCallee(0, size);
    ASSERT_FALSE(buffer.isValid());
    ASSERT_EQ(nullptr, buffer.getData());
    ASSERT_EQ(0u, buffer.getSize());
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_NoBuffsAvailable_SIZE(1);
    ASSERT_EVENTS_NoBuffsAvailable(0, size);
}

void BufferManagerTester ::returnBuffer(Fw::Buffer& buffer) {
    this->clearHistory();
    this->invoke_to_bufferSendIn(0, buffer);
    ASSERT_EVENTS_SIZE(0);
}

void BufferManagerTester ::returnEmptyBuffer(Fw::Buffer& empty) {
    this->clearHistory();
    this->invoke_to_bufferSendIn(0, empty);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_NullEmptyBuffer_SIZE(1);
}

void BufferManagerTester ::checkTelemetry(U32 expectedChannels) {
    this->clearHistory();
    this->invoke_to_schedIn(0, 0);
    ASSERT_TLM_SIZE(expectedChannels);
}

void BufferManagerTester ::setupPool(const BufferManagerComponentImpl::BufferBins& bins, FwSizeType alignment) {
    const U32 liveBefore = this->m_allocator.getLiveCount();
    const U32 deallocsBefore = this->m_allocator.getDeallocCount();
    this->m_poolAlignment = alignment;
    this->component.setup(MGR_ID, MEM_ID, this->m_allocator, bins, alignment);
    // Memory was requested from the supplied allocator under the configured identifier, every request at a non-zero
    // power-of-two alignment and served in full; nothing was released. How many allocations setup() makes is its own
    // business: cleanupPool() checks that each one comes back.
    ASSERT_GT(this->m_allocator.getLiveCount(), liveBefore);
    ASSERT_EQ(deallocsBefore, this->m_allocator.getDeallocCount());
    ASSERT_TRUE(this->m_allocator.allRequestedUnder(MEM_ID));
    ASSERT_TRUE(this->m_allocator.allRequestsWellFormed());
}

void BufferManagerTester ::cleanupPool() {
    this->component.cleanup();
    this->checkAllReleased(this->m_allocator);
}

void BufferManagerTester ::checkAllReleased(const TestAllocator& allocator) {
    // Every allocation ever made through the allocator was released exactly once, under the identifier it was
    // requested with: nothing live, as many releases as requests, none of them unknown, repeated or misidentified
    ASSERT_EQ(0u, allocator.getLiveCount());
    ASSERT_EQ(allocator.getAllocationCount(), allocator.getDeallocCount());
    ASSERT_EQ(0u, allocator.getBadDeallocCount());
    ASSERT_TRUE(allocator.allRequestedUnder(MEM_ID));
}

void BufferManagerTester ::checkAllocationAlignment(const Fw::Buffer& buffer) {
    // A buffer lying at the start of an allocation has the alignment the pool was set up with, and that allocation
    // was requested at a power of two no looser than it: exactly it from the allocator default up, which nothing
    // setup() places in memory exceeds. Where the other buffers lie is the component's business.
    if (this->m_allocator.startsLiveAllocation(buffer.getData())) {
        const FwSizeType requested = this->m_allocator.alignmentRequestedAt(buffer.getData());
        ASSERT_TRUE(BufferManagerTestState::isAligned(buffer, this->m_poolAlignment));
        ASSERT_GE(requested, this->m_poolAlignment);
        if (this->m_poolAlignment >= DEFAULT_ALIGNMENT) {
            ASSERT_EQ(this->m_poolAlignment, requested);
        }
    }
}

void BufferManagerTester ::expectSetupDeath(const BufferManagerComponentImpl::BufferBins& bins, FwSizeType alignment) {
    ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, this->m_allocator, bins, alignment), UUT_ASSERT);
}

void BufferManagerTester ::ruleGet(FwSizeType size) {
    this->clearHistory();
    U16 bin = 0;
    ASSERT_TRUE(this->shadow.expectedBin(size, bin));

    Fw::Buffer buffer = this->invoke_to_bufferGetCallee(0, size);

    ASSERT_NE(nullptr, buffer.getData());
    ASSERT_EQ(size, buffer.getSize());
    ASSERT_EQ(this->shadow.binSize[bin], buffer.getCapacity());
    ASSERT_EQ(static_cast<U32>(MGR_ID), BufferManagerTestState::managerIdOf(buffer));
    ASSERT_FALSE(this->shadow.overlapsHeld(buffer));
    ASSERT_EVENTS_SIZE(0);
    this->checkAllocationAlignment(buffer);

    // Stamp the payload so a later return can detect cross-talk between buffers
    const U8 pattern = static_cast<U8>(STest::Pick::lowerUpper(1, 255));
    (void)memset(buffer.getData(), pattern, buffer.getSize());
    this->shadow.recordGet(buffer, bin, size, pattern);
}

void BufferManagerTester ::ruleReturn(FwSizeType index, Fw::Buffer& buffer) {
    this->clearHistory();
    const BufferManagerTestState::HeldBuffer& held = this->shadow.held[index];
    for (FwSizeType k = 0; k < held.requested; k++) {
        ASSERT_EQ(held.pattern, held.buffer.getData()[k]);
    }

    this->invoke_to_bufferSendIn(0, buffer);

    ASSERT_EVENTS_SIZE(0);
    this->shadow.recordReturn(index);
}

// ----------------------------------------------------------------------
// Harness wiring
// ----------------------------------------------------------------------

void BufferManagerTester ::connectPorts() {
    this->connect_to_bufferSendIn(0, this->component.get_bufferSendIn_InputPort(0));
    this->connect_to_bufferGetCallee(0, this->component.get_bufferGetCallee_InputPort(0));
    this->connect_to_schedIn(0, this->component.get_schedIn_InputPort(0));
    this->component.set_timeCaller_OutputPort(0, this->get_from_timeCaller(0));
    this->component.set_eventOut_OutputPort(0, this->get_from_eventOut(0));
    this->component.set_textEventOut_OutputPort(0, this->get_from_textEventOut(0));
    this->component.set_tlmOut_OutputPort(0, this->get_from_tlmOut(0));
}

void BufferManagerTester ::initComponents() {
    this->init();
    this->component.init(INSTANCE);
}

}  // end namespace Svc
