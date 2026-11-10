// ======================================================================
// \title  Sched.cpp
// \brief  Rule implementations for the Sched rule group
//
// These rules exercise the schedIn port and the telemetry it writes.
// ======================================================================
#include "Svc/BufferManager/test/ut/BufferManagerTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Sched.Telemetry — every channel is written on the first call and then only when its value changed
// ----------------------------------------------------------------------

bool BufferManagerTester::Sched__Telemetry__precondition() const {
    return this->shadow.isSetup;
}

void BufferManagerTester::Sched__Telemetry__action() {
    this->clearHistory();

    this->invoke_to_schedIn(0, 0);

    const bool first = !this->shadow.tlmSent;
    U32 expected = 0;
    if (first || this->shadow.lastTotalBuffs != this->shadow.totalBuffs) {
        ASSERT_TLM_TotalBuffs_SIZE(1);
        ASSERT_TLM_TotalBuffs(0, this->shadow.totalBuffs);
        expected++;
    } else {
        ASSERT_TLM_TotalBuffs_SIZE(0);
    }
    if (first || this->shadow.lastCurrBuffs != this->shadow.currBuffs) {
        ASSERT_TLM_CurrBuffs_SIZE(1);
        ASSERT_TLM_CurrBuffs(0, this->shadow.currBuffs);
        expected++;
    } else {
        ASSERT_TLM_CurrBuffs_SIZE(0);
    }
    if (first || this->shadow.lastHighWater != this->shadow.highWater) {
        ASSERT_TLM_HiBuffs_SIZE(1);
        ASSERT_TLM_HiBuffs(0, this->shadow.highWater);
        expected++;
    } else {
        ASSERT_TLM_HiBuffs_SIZE(0);
    }
    if (first || this->shadow.lastNoBuffs != this->shadow.noBuffs) {
        ASSERT_TLM_NoBuffs_SIZE(1);
        ASSERT_TLM_NoBuffs(0, this->shadow.noBuffs);
        expected++;
    } else {
        ASSERT_TLM_NoBuffs_SIZE(0);
    }
    if (first || this->shadow.lastEmptyBuffs != this->shadow.emptyBuffs) {
        ASSERT_TLM_EmptyBuffs_SIZE(1);
        ASSERT_TLM_EmptyBuffs(0, this->shadow.emptyBuffs);
        expected++;
    } else {
        ASSERT_TLM_EmptyBuffs_SIZE(0);
    }
    ASSERT_TLM_SIZE(expected);

    this->shadow.recordTelemetry();
}

}  // namespace Svc
