// ======================================================================
// \title  Sched.cpp
// \brief  Rule implementations for the Sched rule group
//
// These rules exercise the schedIn port and the telemetry it writes.
// ======================================================================
#include "Svc/BufferManager/test/ut/BufferManagerTester.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Sched.Telemetry
// ----------------------------------------------------------------------

bool BufferManagerTester::Sched__Telemetry__precondition() const {
    return this->shadow.shadow_isSetup;
}

void BufferManagerTester::Sched__Telemetry__action() {
    this->clearHistory();

    this->invoke_to_schedIn(0, 0);

    // Channels are "update on change": written on the first call and then only when the value differs
    const bool first = !this->shadow.shadow_tlmSent;
    U32 expected = 0;
    if (first || this->shadow.shadow_lastTotalBuffs != this->shadow.shadow_totalBuffs) {
        ASSERT_TLM_TotalBuffs_SIZE(1);
        ASSERT_TLM_TotalBuffs(0, this->shadow.shadow_totalBuffs);
        expected++;
    } else {
        ASSERT_TLM_TotalBuffs_SIZE(0);
    }
    if (first || this->shadow.shadow_lastCurrBuffs != this->shadow.shadow_currBuffs) {
        ASSERT_TLM_CurrBuffs_SIZE(1);
        ASSERT_TLM_CurrBuffs(0, this->shadow.shadow_currBuffs);
        expected++;
    } else {
        ASSERT_TLM_CurrBuffs_SIZE(0);
    }
    if (first || this->shadow.shadow_lastHighWater != this->shadow.shadow_highWater) {
        ASSERT_TLM_HiBuffs_SIZE(1);
        ASSERT_TLM_HiBuffs(0, this->shadow.shadow_highWater);
        expected++;
    } else {
        ASSERT_TLM_HiBuffs_SIZE(0);
    }
    if (first || this->shadow.shadow_lastNoBuffs != this->shadow.shadow_noBuffs) {
        ASSERT_TLM_NoBuffs_SIZE(1);
        ASSERT_TLM_NoBuffs(0, this->shadow.shadow_noBuffs);
        expected++;
    } else {
        ASSERT_TLM_NoBuffs_SIZE(0);
    }
    if (first || this->shadow.shadow_lastEmptyBuffs != this->shadow.shadow_emptyBuffs) {
        ASSERT_TLM_EmptyBuffs_SIZE(1);
        ASSERT_TLM_EmptyBuffs(0, this->shadow.shadow_emptyBuffs);
        expected++;
    } else {
        ASSERT_TLM_EmptyBuffs_SIZE(0);
    }
    ASSERT_TLM_SIZE(expected);

    this->shadow.shadow_tlmSent = true;
    this->shadow.shadow_lastTotalBuffs = this->shadow.shadow_totalBuffs;
    this->shadow.shadow_lastCurrBuffs = this->shadow.shadow_currBuffs;
    this->shadow.shadow_lastHighWater = this->shadow.shadow_highWater;
    this->shadow.shadow_lastNoBuffs = this->shadow.shadow_noBuffs;
    this->shadow.shadow_lastEmptyBuffs = this->shadow.shadow_emptyBuffs;
}

}  // namespace Svc
