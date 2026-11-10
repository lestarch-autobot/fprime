// ======================================================================
// \title  SET_PROC_OVERRIDE.cpp
// \author lestarch-autobot
// \brief  SET_PROC_OVERRIDE class implementation
// ======================================================================

#include "Svc/DpWriter/test/ut/Rules/SET_PROC_OVERRIDE.hpp"
#include "STest/Pick/Pick.hpp"
#include "Svc/DpWriter/test/ut/Rules/Testers.hpp"

namespace Svc {

namespace {

//! Pick an override state at random
Fw::Enabled::T pickState() {
    return static_cast<bool>(STest::Pick::lowerUpper(0, 1)) ? Fw::Enabled::ENABLED : Fw::Enabled::DISABLED;
}

}  // namespace

// ----------------------------------------------------------------------
// Rule definitions
// ----------------------------------------------------------------------

bool TestState ::precondition__SET_PROC_OVERRIDE__OK() const {
    return true;
}

void TestState ::action__SET_PROC_OVERRIDE__OK() {
    // Clear history
    this->clearHistory();
    // Pick a state and a mask within the allowed bits, unless the test steered them
    const Fw::Enabled state(this->abstractState.m_nextProcOverrideStateOpt.value_or(pickState()));
    const Fw::DpCfg::ProcType::SerialType allowed = AbstractState::getAllowedProcMask(state);
    const Fw::DpCfg::ProcType::SerialType mask = this->abstractState.m_nextProcOverrideMaskOpt.value_or(
        static_cast<Fw::DpCfg::ProcType::SerialType>(STest::Pick::any() & allowed));
    this->abstractState.m_nextProcOverrideStateOpt.reset();
    this->abstractState.m_nextProcOverrideMaskOpt.reset();
    // Send the command
    const U32 cmdSeq = STest::Pick::any();
    this->sendCmd_SET_PROC_OVERRIDE(0, cmdSeq, state, static_cast<U8>(mask));
    this->doDispatch();
    // Check the command response
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, DpWriterTester::getOpCodeSetProcOverride(), cmdSeq, Fw::CmdResponse::OK);
    // Check events
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_ProcOverrideSet_SIZE(1);
    ASSERT_EVENTS_ProcOverrideSet(0, state, static_cast<U8>(mask));
    // Update the abstract state
    this->abstractState.m_procOverrideState = state;
    this->abstractState.m_procOverrideMask = mask;
    this->abstractState.m_ProcOverrideState.value = state.e;
    this->abstractState.m_ProcOverrideMask.value = static_cast<U8>(mask);
}

bool TestState ::precondition__SET_PROC_OVERRIDE__Invalid() const {
    // Some bit must be disallowed in at least the ENABLED state
    return static_cast<U8>(~AbstractState::getAllowedProcMask(Fw::Enabled::ENABLED)) != 0;
}

void TestState ::action__SET_PROC_OVERRIDE__Invalid() {
    // Clear history
    this->clearHistory();
    // Pick a state that has a disallowed bit
    Fw::Enabled state(pickState());
    Fw::DpCfg::ProcType::SerialType allowed = AbstractState::getAllowedProcMask(state);
    if (static_cast<U8>(~allowed) == 0) {
        state = Fw::Enabled::ENABLED;
        allowed = AbstractState::getAllowedProcMask(state);
    }
    const U8 disallowed = static_cast<U8>(~allowed);
    // Pick a mask with at least one disallowed bit
    U8 mask = static_cast<U8>(STest::Pick::any() & disallowed);
    while (mask == 0) {
        mask = static_cast<U8>(STest::Pick::any() & disallowed);
    }
    mask = static_cast<U8>(mask | static_cast<U8>(STest::Pick::any() & allowed));
    // Send the command
    const U32 cmdSeq = STest::Pick::any();
    this->sendCmd_SET_PROC_OVERRIDE(0, cmdSeq, state, mask);
    this->doDispatch();
    // Check the command response
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, DpWriterTester::getOpCodeSetProcOverride(), cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
    // Check events
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_ProcOverrideInvalid_SIZE(1);
    ASSERT_EVENTS_ProcOverrideInvalid(0, state, mask, static_cast<U8>(allowed));
    // The override is unchanged: the modeled state is left as is
}

namespace SET_PROC_OVERRIDE {

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void Tester ::OK() {
    this->ruleOK.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
}

void Tester ::Invalid() {
    this->ruleInvalid.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
}

void Tester ::ForceOn() {
    this->testState.setNextProcOverride(Fw::Enabled::ENABLED, AbstractState::getConnectedProcMask());
    this->ruleOK.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
    Testers::bufferSendIn.ruleOK.apply(this->testState);
    Testers::bufferSendIn.ruleOKProcShrink.apply(this->testState);
}

void Tester ::ForceOff() {
    this->testState.setNextProcOverride(Fw::Enabled::DISABLED, AbstractState::getAllProcMask());
    this->ruleOK.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
    Testers::bufferSendIn.ruleOK.apply(this->testState);
    Testers::bufferSendIn.ruleOKProcShrink.apply(this->testState);
}

void Tester ::Clear() {
    this->ForceOff();
    this->testState.setNextProcOverride(Fw::Enabled::ENABLED, 0);
    this->ruleOK.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
    Testers::bufferSendIn.ruleOK.apply(this->testState);
    Testers::bufferSendIn.ruleOKProcShrink.apply(this->testState);
}

}  // namespace SET_PROC_OVERRIDE

}  // namespace Svc
