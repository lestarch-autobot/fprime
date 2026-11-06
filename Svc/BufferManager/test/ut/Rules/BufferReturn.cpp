// ======================================================================
// \title  BufferReturn.cpp
// \brief  Rule implementations for the BufferReturn rule group
//
// These rules exercise the bufferSendIn port.
// ======================================================================
#include "Svc/BufferManager/test/ut/BufferManagerTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// BufferReturn.Ok
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferReturn__Ok__precondition() const {
    return !this->shadow.shadow_held.empty();
}

void BufferManagerTester::BufferReturn__Ok__action() {
    this->clearHistory();
    const FwSizeType index = this->shadow.pickHeldIndex();
    Fw::Buffer buffer = this->shadow.shadow_held[index].buffer;

    this->invoke_to_bufferSendIn(0, buffer);

    ASSERT_EVENTS_SIZE(0);
    this->shadow.recordReturn(index);
}

// ----------------------------------------------------------------------
// BufferReturn.Trimmed
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferReturn__Trimmed__precondition() const {
    return !this->shadow.shadow_held.empty();
}

void BufferManagerTester::BufferReturn__Trimmed__action() {
    this->clearHistory();
    const FwSizeType index = this->shadow.pickHeldIndex();
    Fw::Buffer buffer = this->shadow.shadow_held[index].buffer;
    // The user may hand back less than was requested
    buffer.setSize(STest::Pick::lowerUpper(0, static_cast<U32>(this->shadow.shadow_held[index].requested)));

    this->invoke_to_bufferSendIn(0, buffer);

    ASSERT_EVENTS_SIZE(0);
    this->shadow.recordReturn(index);
}

// ----------------------------------------------------------------------
// BufferReturn.Empty
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferReturn__Empty__precondition() const {
    return true;
}

void BufferManagerTester::BufferReturn__Empty__action() {
    this->clearHistory();
    Fw::Buffer empty;

    this->invoke_to_bufferSendIn(0, empty);

    const U32 eventsBefore = this->shadow.shadow_emptyBuffsEvents;
    this->shadow.recordEmptyReturn();
    if (eventsBefore < BufferManagerTestState::EVENT_THROTTLE) {
        ASSERT_EVENTS_SIZE(1);
        ASSERT_EVENTS_NullEmptyBuffer_SIZE(1);
    } else {
        ASSERT_EVENTS_SIZE(0);
    }
}

}  // namespace Svc
