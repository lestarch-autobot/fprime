// ======================================================================
// \title  BufferGet.cpp
// \brief  Rule implementations for the BufferGet rule group
//
// These rules exercise the bufferGetCallee port.
// ======================================================================
#include "Svc/BufferManager/test/ut/BufferManagerTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// BufferGet.Ok
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferGet__Ok__precondition() const {
    return this->shadow.anyFree();
}

void BufferManagerTester::BufferGet__Ok__action() {
    this->clearHistory();
    const FwSizeType size = this->shadow.pickServableSize();
    U16 bin = 0;
    ASSERT_TRUE(this->shadow.expectedBin(size, bin));

    Fw::Buffer buffer = this->invoke_to_bufferGetCallee(0, size);

    ASSERT_TRUE(buffer.isValid());
    ASSERT_EQ(size, buffer.getSize());
    ASSERT_EQ(this->shadow.shadow_binSize[bin], buffer.getCapacity());
    ASSERT_EQ(static_cast<U32>(MGR_ID), buffer.getContext() >> 16);
    ASSERT_FALSE(this->shadow.overlapsHeld(buffer));
    ASSERT_EVENTS_SIZE(0);

    this->shadow.recordGet(buffer, bin, size);
}

// ----------------------------------------------------------------------
// BufferGet.NoneAvailable
// ----------------------------------------------------------------------

bool BufferManagerTester::BufferGet__NoneAvailable__precondition() const {
    return true;
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
