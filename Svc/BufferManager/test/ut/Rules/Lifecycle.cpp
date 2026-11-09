// ======================================================================
// \title  Lifecycle.cpp
// \brief  Rule implementations for the Lifecycle rule group
//
// These rules exercise setup() and cleanup() and the checks they perform.
// ======================================================================
#include <cstring>
#include <limits>
#include "Svc/BufferManager/test/ut/BufferManagerTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Lifecycle.Setup — set up with the staged bins and check the allocator contract
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__Setup__precondition() const {
    return !this->shadow.shadow_everSetup;
}

void BufferManagerTester::Lifecycle__Setup__action() {
    // Copy: configure() below resets the whole shadow, including shadow_bins
    const BufferManagerComponentImpl::BufferBins bins = this->shadow.shadow_bins;
    this->component.setup(MGR_ID, MEM_ID, this->m_rulesAllocator, bins);
    this->shadow.configure(bins);
    this->clearHistory();

    // One allocation, with the configured identifier, large enough for every payload
    ASSERT_EQ(MEM_ID, this->m_rulesAllocator.getId());
    ASSERT_NE(nullptr, this->m_rulesAllocator.getMem());
    ASSERT_GE(this->m_rulesAllocator.getSize(), this->shadow.totalPayloadBytes());
    ASSERT_EQ(0u, this->m_rulesAllocator.getDeallocCount());
}

// ----------------------------------------------------------------------
// Lifecycle.Cleanup — return everything, clean up, and check the release happens once
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__Cleanup__precondition() const {
    return this->shadow.shadow_isSetup;
}

void BufferManagerTester::Lifecycle__Cleanup__action() {
    while (!this->shadow.shadow_held.empty()) {
        this->BufferReturn__Ok__action();
    }
    const U32 before = this->m_rulesAllocator.getDeallocCount();
    this->component.cleanup();
    this->component.cleanup();  // second call must be a no-op
    ASSERT_EQ(before + 1, this->m_rulesAllocator.getDeallocCount());
    ASSERT_EQ(this->m_rulesAllocator.getMem(), this->m_rulesAllocator.getDeallocPtr());
    this->shadow.shadow_isSetup = false;
}

// ----------------------------------------------------------------------
// Lifecycle.NotSetupAsserts — the ports reject use while not set up
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__NotSetupAsserts__precondition() const {
    return !this->shadow.shadow_isSetup;
}

void BufferManagerTester::Lifecycle__NotSetupAsserts__action() {
    Fw::Buffer empty;
    ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferGetCallee(0, ANY_REQUEST_SIZE), "Assert: ");
    ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, empty), "Assert: ");
    if (!this->shadow.shadow_everSetup) {
        ASSERT_DEATH_IF_SUPPORTED(this->component.cleanup(), "Assert: ");
    }
}

// ----------------------------------------------------------------------
// Lifecycle.BinOrderAsserts — bins not in non-decreasing size order are rejected
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__BinOrderAsserts__precondition() const {
    return !this->shadow.shadow_everSetup;
}

void BufferManagerTester::Lifecycle__BinOrderAsserts__action() {
    BufferManagerComponentImpl::BufferBins bins;
    // Descending sizes are rejected
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[0].numBuffers = 1;
    bins.bins[1].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[1].numBuffers = 1;
    ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, this->m_rulesAllocator, bins), "Assert: ");
    // An empty bin between them does not hide the descending pair
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = BIN1_BUFFER_SIZE;
    bins.bins[0].numBuffers = 1;
    bins.bins[2].bufferSize = BIN0_BUFFER_SIZE;
    bins.bins[2].numBuffers = 1;
    ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, this->m_rulesAllocator, bins), "Assert: ");
}

// ----------------------------------------------------------------------
// Lifecycle.SizeOverflowAsserts — arithmetic that would wrap is rejected
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__SizeOverflowAsserts__precondition() const {
    return !this->shadow.shadow_everSetup;
}

void BufferManagerTester::Lifecycle__SizeOverflowAsserts__action() {
    const FwSizeType maxSize = std::numeric_limits<FwSizeType>::max();
    BufferManagerComponentImpl::BufferBins bins;
    // bufferSize + sizeof(AllocatedBuffer) wraps
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = maxSize;
    bins.bins[0].numBuffers = 1;
    ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, this->m_rulesAllocator, bins), "Assert: ");
    // per-buffer size times numBuffers wraps
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = maxSize / 2;
    bins.bins[0].numBuffers = 3;
    ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, this->m_rulesAllocator, bins), "Assert: ");
    // the accumulation across bins wraps
    memset(&bins, 0, sizeof(bins));
    bins.bins[0].bufferSize = maxSize / 2;
    bins.bins[0].numBuffers = 1;
    bins.bins[1].bufferSize = maxSize / 2;
    bins.bins[1].numBuffers = 1;
    ASSERT_DEATH_IF_SUPPORTED(this->component.setup(MGR_ID, MEM_ID, this->m_rulesAllocator, bins), "Assert: ");
}

}  // namespace Svc
