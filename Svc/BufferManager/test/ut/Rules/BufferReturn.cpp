// ======================================================================
// \title  BufferReturn.cpp
// \brief  Rule implementations for the BufferReturn rule group
//
// These rules exercise the bufferSendIn port.
// ======================================================================
#include "STest/Pick/Pick.hpp"
#include "Svc/BufferManager/test/ut/BufferManagerTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// BufferReturn.Ok — a held buffer goes back as it was handed out
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferReturn__Ok__precondition() const {
    return this->shadow.isSetup && !this->shadow.held.empty();
}

void BufferManagerTester::BufferReturn__Ok__action() {
    const FwSizeType index = this->shadow.pickHeldIndex();
    Fw::Buffer buffer = this->shadow.held[index].buffer;
    this->ruleReturn(index, buffer);
}

// ----------------------------------------------------------------------
// BufferReturn.Trimmed — the user hands back less than was requested
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferReturn__Trimmed__precondition() const {
    return this->shadow.isSetup && !this->shadow.held.empty();
}

void BufferManagerTester::BufferReturn__Trimmed__action() {
    const FwSizeType index = this->shadow.pickHeldIndex();
    Fw::Buffer buffer = this->shadow.held[index].buffer;
    buffer.setSize(STest::Pick::lowerUpper(0, static_cast<U32>(this->shadow.held[index].requested)));
    this->ruleReturn(index, buffer);
}

// ----------------------------------------------------------------------
// BufferReturn.Empty — an empty buffer is refused with a warning
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferReturn__Empty__precondition() const {
    return this->shadow.isSetup;
}

void BufferManagerTester::BufferReturn__Empty__action() {
    this->clearHistory();
    Fw::Buffer empty;

    this->invoke_to_bufferSendIn(0, empty);

    const U32 eventsBefore = this->shadow.emptyBuffsEvents;
    this->shadow.recordEmptyReturn();
    if (eventsBefore < BufferManagerTestState::EVENT_THROTTLE) {
        ASSERT_EVENTS_SIZE(1);
        ASSERT_EVENTS_NullEmptyBuffer_SIZE(1);
    } else {
        ASSERT_EVENTS_SIZE(0);
    }
}

// ----------------------------------------------------------------------
// BufferReturn.Asserts — corrupt or foreign buffers are rejected
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferReturn__Asserts__precondition() const {
    // At least one buffer out and at least one still in the pool
    return this->shadow.isSetup && !this->shadow.held.empty() && this->shadow.held.size() < this->shadow.totalBuffs;
}

void BufferManagerTester::BufferReturn__Asserts__action() {
    this->clearHistory();
    const FwSizeType index = this->shadow.pickHeldIndex();
    Fw::Buffer held = this->shadow.held[index].buffer;
    const U32 id = BufferManagerTestState::bufferIdOf(held);
    U8 foreign[1] = {};

    // Wrong manager ID
    {
        Fw::Buffer bad = held;
        bad.setContext(BufferManagerTestState::contextOf(MGR_ID + 1, id));
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, bad), UUT_ASSERT);
    }
    // Buffer ID beyond the pool
    {
        Fw::Buffer bad = held;
        bad.setContext(BufferManagerTestState::contextOf(MGR_ID, this->shadow.totalBuffs));
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, bad), UUT_ASSERT);
    }
    // Buffer ID of a buffer that is not out
    bool foundFree = false;
    for (U32 other = 0; other < this->shadow.totalBuffs && !foundFree; other++) {
        if (!this->shadow.isHeldId(other)) {
            Fw::Buffer bad(held.getData(), held.getSize(), BufferManagerTestState::contextOf(MGR_ID, other));
            ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, bad), UUT_ASSERT);
            foundFree = true;
        }
    }
    ASSERT_TRUE(foundFree);
    // Pointer outside the buffer's own memory
    {
        Fw::Buffer bad(foreign, 1, held.getContext());
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, bad), UUT_ASSERT);
    }
    // Size larger than the buffer's allocation
    {
        Fw::Buffer bad(held.getData(), held.getCapacity() + 1, held.getContext());
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, bad), UUT_ASSERT);
    }
    // Null data with a non-zero size is not the empty buffer and is rejected
    {
        Fw::Buffer bad(nullptr, held.getCapacity(), held.getContext());
        ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, bad), UUT_ASSERT);
    }
    // Returning the same buffer twice
    this->ruleReturn(index, held);
    ASSERT_DEATH_IF_SUPPORTED(this->invoke_to_bufferSendIn(0, held), UUT_ASSERT);
}

}  // namespace Svc
