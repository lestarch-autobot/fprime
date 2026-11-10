// ======================================================================
// \title  Lifecycle.cpp
// \brief  Rule implementations for the Lifecycle rule group
//
// These rules exercise setup() and cleanup() and the checks they perform.
// ======================================================================
#include <limits>
#include "Svc/BufferManager/test/ut/BufferManagerTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Lifecycle.Setup — set up with the staged bins and check the allocator contract
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__Setup__precondition() const {
    return !this->shadow.isSetup;
}

void BufferManagerTester::Lifecycle__Setup__action() {
    const U32 deallocsBefore = this->m_allocator.getDeallocCount();
    this->component.setup(MGR_ID, MEM_ID, this->m_allocator, this->shadow.table, this->shadow.alignment);
    this->shadow.configure();
    this->clearHistory();

    // The pool was requested from the configured allocator under the configured identifier, at a power-of-two
    // alignment no looser than the one staged (exactly it from the allocator default up); nothing was released
    ASSERT_EQ(MEM_ID, this->m_allocator.getId());
    ASSERT_NE(nullptr, this->m_allocator.getMem());
    ASSERT_TRUE(BufferManagerTestState::isPowerOfTwo(this->m_allocator.getAlignment()));
    ASSERT_GE(this->m_allocator.getAlignment(), this->shadow.alignment);
    if (this->shadow.alignment >= DEFAULT_ALIGNMENT) {
        ASSERT_EQ(this->shadow.alignment, this->m_allocator.getAlignment());
    }
    ASSERT_EQ(deallocsBefore, this->m_allocator.getDeallocCount());
}

// ----------------------------------------------------------------------
// Lifecycle.Cleanup — return everything, clean up, and check the release happens once
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__Cleanup__precondition() const {
    return this->shadow.isSetup;
}

void BufferManagerTester::Lifecycle__Cleanup__action() {
    while (!this->shadow.held.empty()) {
        this->BufferReturn__Ok__action();
    }
    const U32 before = this->m_allocator.getDeallocCount();
    this->component.cleanup();
    this->component.cleanup();  // second call must be a no-op
    ASSERT_EQ(before + 1, this->m_allocator.getDeallocCount());
    ASSERT_EQ(this->m_allocator.getMem(), this->m_allocator.getDeallocPtr());
    this->shadow.recordCleanup();
}

// ----------------------------------------------------------------------
// Lifecycle.CleanupHolding — clean up while buffers are still out; the pool goes away with them
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__CleanupHolding__precondition() const {
    return this->shadow.isSetup && !this->shadow.held.empty();
}

void BufferManagerTester::Lifecycle__CleanupHolding__action() {
    const U32 before = this->m_allocator.getDeallocCount();
    this->component.cleanup();
    ASSERT_EQ(before + 1, this->m_allocator.getDeallocCount());
    ASSERT_EQ(this->m_allocator.getMem(), this->m_allocator.getDeallocPtr());
    this->shadow.recordCleanup();
}

// ----------------------------------------------------------------------
// Lifecycle.NotSetupAsserts — the ports reject use while not set up; so does cleanup() before any setup()
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__NotSetupAsserts__precondition() const {
    return !this->shadow.isSetup;
}

void BufferManagerTester::Lifecycle__NotSetupAsserts__action() {
    Fw::Buffer empty;
    ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferGetCallee(0, ANY_REQUEST_SIZE), UUT_ASSERT);
    ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, empty), UUT_ASSERT);
    if (!this->shadow.everSetup) {
        ASSERT_DEATH_IF_SUPPORTED(this->component.cleanup(), UUT_ASSERT);
    }
}

// ----------------------------------------------------------------------
// Lifecycle.BinOrderAsserts — bins not in non-decreasing size order are rejected
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__BinOrderAsserts__precondition() const {
    return !this->shadow.isSetup;
}

void BufferManagerTester::Lifecycle__BinOrderAsserts__action() {
    // Descending sizes are rejected
    this->expectSetupDeath(twoBins(BIN1_BUFFER_SIZE, 1, 1, BIN0_BUFFER_SIZE, 1));
    // An empty bin between them does not hide the descending pair
    this->expectSetupDeath(twoBins(BIN1_BUFFER_SIZE, 1, 2, BIN0_BUFFER_SIZE, 1));
}

// ----------------------------------------------------------------------
// Lifecycle.EmptyBinsAsserts — a configuration with no buffers, or with zero-sized buffers, is rejected
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__EmptyBinsAsserts__precondition() const {
    return !this->shadow.isSetup;
}

void BufferManagerTester::Lifecycle__EmptyBinsAsserts__action() {
    BufferManagerComponentImpl::BufferBins bins = {};
    this->expectSetupDeath(bins);
    // a size without any buffers is still empty
    bins.bins[0].bufferSize = BIN0_BUFFER_SIZE;
    this->expectSetupDeath(bins);
    // buffers without a size can never serve a request
    this->expectSetupDeath(oneBin(0, 1));
}

// ----------------------------------------------------------------------
// Lifecycle.SetupTwiceAsserts — setting up over a live pool is rejected
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__SetupTwiceAsserts__precondition() const {
    return this->shadow.isSetup;
}

void BufferManagerTester::Lifecycle__SetupTwiceAsserts__action() {
    this->expectSetupDeath(this->shadow.table);
}

// ----------------------------------------------------------------------
// Lifecycle.AlignmentAsserts — an alignment that is zero or not a power of two is rejected, and so is memory the
// allocator hands back without the alignment asked of it
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__AlignmentAsserts__precondition() const {
    return !this->shadow.isSetup;
}

void BufferManagerTester::Lifecycle__AlignmentAsserts__action() {
    this->expectSetupDeath(defaultBins(), 0);                            // zero
    this->expectSetupDeath(defaultBins(), 3);                            // not a power of two
    this->expectSetupDeath(defaultBins(), ABOVE_DEFAULT_ALIGNMENT + 1);  // one past a power of two
    // An allocator that does not honor the alignment (even the default) is caught before the pool is used
    this->m_allocator.setMisaligned(true);
    this->expectSetupDeath(defaultBins());
    this->m_allocator.setMisaligned(false);
}

// ----------------------------------------------------------------------
// Lifecycle.SizeOverflowAsserts — size arithmetic that would wrap is rejected
// ----------------------------------------------------------------------

bool BufferManagerTester::Lifecycle__SizeOverflowAsserts__precondition() const {
    return !this->shadow.isSetup;
}

void BufferManagerTester::Lifecycle__SizeOverflowAsserts__action() {
    // Each step of setup()'s size arithmetic is checked before it can wrap; these inputs wrap for any
    // per-buffer overhead
    const FwSizeType maxSize = std::numeric_limits<FwSizeType>::max();
    // bufferSize plus the per-buffer overhead wraps
    this->expectSetupDeath(oneBin(maxSize, 1));
    // per-buffer size times numBuffers wraps
    this->expectSetupDeath(oneBin(maxSize / 2, 3));
    // the running total across bins wraps
    this->expectSetupDeath(twoBins(maxSize / 2, 1, 1, maxSize / 2, 1));
}

}  // namespace Svc
