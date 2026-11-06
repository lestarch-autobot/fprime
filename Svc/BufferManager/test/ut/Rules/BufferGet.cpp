// ======================================================================
// \title  BufferGet.cpp
// \brief  Rule implementations for the BufferGet rule group
//
// These rules exercise the bufferGetCallee port.
// ======================================================================
#include "Svc/BufferManager/test/ut/BufferManagerTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// BufferGet.Ok — a servable size, biased to the bin boundaries
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferGet__Ok__precondition() const {
    return this->shadow.shadow_isSetup && this->shadow.anyFree();
}

void BufferManagerTester::BufferGet__Ok__action() {
    this->clearHistory();
    this->ruleGet(this->shadow.pickServableSize());
}

// ----------------------------------------------------------------------
// BufferGet.Between — a size strictly between two adjacent bin sizes
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferGet__Between__precondition() const {
    return this->shadow.shadow_isSetup && this->shadow.pickBetweenBinSize() != 0;
}

void BufferManagerTester::BufferGet__Between__action() {
    this->clearHistory();
    this->ruleGet(this->shadow.pickBetweenBinSize());
}

// ----------------------------------------------------------------------
// BufferGet.Zero — a size-0 request reports size 0 and can be handed back
//
// Characterization of current behavior only: nothing is recorded in the shadow, so
// apply it last in a sequence and keep it out of random scenarios.
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferGet__Zero__precondition() const {
    return this->shadow.shadow_isSetup && this->shadow.anyFree();
}

void BufferManagerTester::BufferGet__Zero__action() {
    this->clearHistory();
    Fw::Buffer zero = this->invoke_to_bufferGetCallee(0, 0);
    ASSERT_EQ(0u, zero.getSize());
    this->invoke_to_bufferSendIn(0, zero);
}

// ----------------------------------------------------------------------
// BufferGet.NoneAvailable
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferGet__NoneAvailable__precondition() const {
    return this->shadow.shadow_isSetup;
}

void BufferManagerTester::BufferGet__NoneAvailable__action() {
    this->clearHistory();
    const FwSizeType size = this->shadow.pickUnservableSize();
    U16 bin = 0;
    ASSERT_FALSE(this->shadow.expectedBin(size, bin));

    Fw::Buffer buffer = this->invoke_to_bufferGetCallee(0, size);

    ASSERT_FALSE(buffer.isValid());
    ASSERT_EQ(nullptr, buffer.getData());
    ASSERT_EQ(0u, buffer.getSize());

    const U32 eventsBefore = this->shadow.shadow_noBuffsEvents;
    this->shadow.recordNoBuffer();
    if (eventsBefore < BufferManagerTestState::EVENT_THROTTLE) {
        ASSERT_EVENTS_SIZE(1);
        ASSERT_EVENTS_NoBuffsAvailable_SIZE(1);
        ASSERT_EVENTS_NoBuffsAvailable(0, size);
    } else {
        ASSERT_EVENTS_SIZE(0);
    }
}

}  // namespace Svc
