// ======================================================================
// \title  ProcOverride.cpp
// \author lestarch-autobot
// \brief  ProcOverride class implementation
// ======================================================================

#include "Svc/DpWriter/test/ut/Rules/ProcOverride.hpp"

#include <limits>

#include "STest/Pick/Pick.hpp"
#include "Svc/DpWriter/test/ut/Rules/Testers.hpp"

namespace Svc {

namespace {

//! Pick an override state at random
Fw::Enabled::T pickState() {
    return static_cast<bool>(STest::Pick::lowerUpper(0, 1)) ? Fw::Enabled::ENABLED : Fw::Enabled::DISABLED;
}

//! The processing type enum constants
constexpr Fw::DpCfg::ProcType::T PROC_TYPES[] = {
    Fw::DpCfg::ProcType::PROC_TYPE_NONE,
    Fw::DpCfg::ProcType::PROC_TYPE_ZLIB_DEFLATE,
    Fw::DpCfg::ProcType::PROC_TYPE_ONE,
    Fw::DpCfg::ProcType::PROC_TYPE_TWO,
};
static_assert(FW_NUM_ARRAY_ELEMENTS(PROC_TYPES) == Fw::DpCfg::ProcType::NUM_CONSTANTS,
              "PROC_TYPES must list every Fw::DpCfg::ProcType constant");

//! Pick a processing type whose bits all have a processing port
Fw::DpCfg::ProcType::T pickProcType() {
    Fw::DpCfg::ProcType::T result = Fw::DpCfg::ProcType::PROC_TYPE_NONE;
    const Fw::DpCfg::ProcType::SerialType configured = Svc::AbstractState::getAllProcMask();
    // Bounded retry: PROC_TYPE_NONE is always acceptable
    for (FwSizeType i = 0; i < 2 * FW_NUM_ARRAY_ELEMENTS(PROC_TYPES); i++) {
        const Fw::DpCfg::ProcType::T candidate =
            PROC_TYPES[STest::Pick::lowerUpper(0, static_cast<U32>(FW_NUM_ARRAY_ELEMENTS(PROC_TYPES) - 1))];
        if ((static_cast<Fw::DpCfg::ProcType::SerialType>(candidate) & static_cast<U8>(~configured)) == 0) {
            result = candidate;
            break;
        }
    }
    return result;
}

}  // namespace

// ----------------------------------------------------------------------
// Rule definitions
// ----------------------------------------------------------------------

bool TestState ::precondition__ProcOverride__State() const {
    return true;
}

void TestState ::action__ProcOverride__State() {
    this->clearHistory();
    const Fw::Enabled state(this->abstractState.m_nextProcOverrideStateOpt.value_or(pickState()));
    this->abstractState.m_nextProcOverrideStateOpt.reset();
    const U32 cmdSeq = STest::Pick::any();
    this->sendCmd_SET_PROC_OVERRIDE_STATE(0, cmdSeq, state);
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, DpWriterTester::getOpCodeSetProcOverrideState(), cmdSeq, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_ProcOverrideSet_SIZE(1);
    ASSERT_EVENTS_ProcOverrideSet(0, state, static_cast<U8>(this->abstractState.m_modelProcOverrideMask));
    this->abstractState.m_modelProcOverrideState = state;
    this->abstractState.m_ProcOverrideState.value = state.e;
}

bool TestState ::precondition__ProcOverride__MaskOK() const {
    return true;
}

void TestState ::action__ProcOverride__MaskOK() {
    this->clearHistory();
    const Fw::DpCfg::ProcType::SerialType mask = this->abstractState.m_nextProcOverrideMaskOpt.value_or(
        static_cast<Fw::DpCfg::ProcType::SerialType>(STest::Pick::any() & AbstractState::getAllProcMask()));
    this->abstractState.m_nextProcOverrideMaskOpt.reset();
    const U32 cmdSeq = STest::Pick::any();
    this->sendCmd_SET_PROC_OVERRIDE_MASK(0, cmdSeq, static_cast<U8>(mask));
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, DpWriterTester::getOpCodeSetProcOverrideMask(), cmdSeq, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_ProcOverrideSet_SIZE(1);
    ASSERT_EVENTS_ProcOverrideSet(0, this->abstractState.m_modelProcOverrideState, static_cast<U8>(mask));
    this->abstractState.m_modelProcOverrideMask = mask;
    this->abstractState.m_ProcOverrideMask.value = static_cast<U8>(mask);
}

bool TestState ::precondition__ProcOverride__MaskInvalid() const {
    // Some bit of the U8 mask must have no processing port
    return static_cast<U8>(~AbstractState::getAllProcMask()) != 0;
}

void TestState ::action__ProcOverride__MaskInvalid() {
    this->clearHistory();
    const Fw::DpCfg::ProcType::SerialType configured = AbstractState::getAllProcMask();
    const U8 unconfigured = static_cast<U8>(~configured);
    U8 mask = 0;
    if (this->abstractState.m_nextProcOverrideMaskOpt.has_value()) {
        mask = static_cast<U8>(this->abstractState.m_nextProcOverrideMaskOpt.value());
        ASSERT_NE(static_cast<U8>(mask & unconfigured), 0);
    } else {
        // Pick one unconfigured bit, then any other bits
        FwIndexType numUnconfigured = 0;
        for (FwIndexType i = 0; i < std::numeric_limits<U8>::digits; i++) {
            if ((unconfigured & (1U << i)) != 0) {
                numUnconfigured = static_cast<FwIndexType>(numUnconfigured + 1);
            }
        }
        ASSERT_GT(numUnconfigured, 0);
        FwIndexType pick = static_cast<FwIndexType>(STest::Pick::lowerUpper(0, static_cast<U32>(numUnconfigured - 1)));
        for (FwIndexType i = 0; i < std::numeric_limits<U8>::digits; i++) {
            if ((unconfigured & (1U << i)) != 0) {
                if (pick == 0) {
                    mask = static_cast<U8>(1U << i);
                }
                pick = static_cast<FwIndexType>(pick - 1);
            }
        }
        mask = static_cast<U8>(mask | static_cast<U8>(STest::Pick::any()));
    }
    this->abstractState.m_nextProcOverrideMaskOpt.reset();
    const U32 cmdSeq = STest::Pick::any();
    this->sendCmd_SET_PROC_OVERRIDE_MASK(0, cmdSeq, mask);
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, DpWriterTester::getOpCodeSetProcOverrideMask(), cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_ProcOverrideInvalid_SIZE(1);
    ASSERT_EVENTS_ProcOverrideInvalid(0, mask, static_cast<U8>(configured));
    // The override is unchanged: the modeled state is left as is
}

bool TestState ::precondition__ProcOverride__Add() const {
    return true;
}

void TestState ::action__ProcOverride__Add() {
    this->clearHistory();
    const Fw::DpCfg::ProcType procType(this->abstractState.m_nextProcTypeOpt.value_or(pickProcType()));
    this->abstractState.m_nextProcTypeOpt.reset();
    const Fw::DpCfg::ProcType::SerialType mask = static_cast<Fw::DpCfg::ProcType::SerialType>(
        this->abstractState.m_modelProcOverrideMask | static_cast<Fw::DpCfg::ProcType::SerialType>(procType.e));
    const U32 cmdSeq = STest::Pick::any();
    this->sendCmd_ADD_PROC_OVERRIDE(0, cmdSeq, procType);
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, DpWriterTester::getOpCodeAddProcOverride(), cmdSeq, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_ProcOverrideSet_SIZE(1);
    ASSERT_EVENTS_ProcOverrideSet(0, this->abstractState.m_modelProcOverrideState, static_cast<U8>(mask));
    this->abstractState.m_modelProcOverrideMask = mask;
    this->abstractState.m_ProcOverrideMask.value = static_cast<U8>(mask);
}

bool TestState ::precondition__ProcOverride__Remove() const {
    return true;
}

void TestState ::action__ProcOverride__Remove() {
    this->clearHistory();
    const Fw::DpCfg::ProcType procType(this->abstractState.m_nextProcTypeOpt.value_or(pickProcType()));
    this->abstractState.m_nextProcTypeOpt.reset();
    const Fw::DpCfg::ProcType::SerialType mask = static_cast<Fw::DpCfg::ProcType::SerialType>(
        this->abstractState.m_modelProcOverrideMask &
        static_cast<Fw::DpCfg::ProcType::SerialType>(~static_cast<Fw::DpCfg::ProcType::SerialType>(procType.e)));
    const U32 cmdSeq = STest::Pick::any();
    this->sendCmd_REMOVE_PROC_OVERRIDE(0, cmdSeq, procType);
    this->doDispatch();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, DpWriterTester::getOpCodeRemoveProcOverride(), cmdSeq, Fw::CmdResponse::OK);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_ProcOverrideSet_SIZE(1);
    ASSERT_EVENTS_ProcOverrideSet(0, this->abstractState.m_modelProcOverrideState, static_cast<U8>(mask));
    this->abstractState.m_modelProcOverrideMask = mask;
    this->abstractState.m_ProcOverrideMask.value = static_cast<U8>(mask);
}

namespace ProcOverride {

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void Tester ::State() {
    this->ruleState.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
    // A second tick with an unchanged override must emit neither channel
    Testers::schedIn.ruleOK.apply(this->testState);
}

void Tester ::MaskOK() {
    this->ruleMaskOK.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
}

void Tester ::MaskInvalid() {
    this->ruleMaskInvalid.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
}

void Tester ::MaskInvalidKeepsMask() {
    this->ForceOn();
    this->testState.setNextProcOverrideMask(static_cast<Fw::DpCfg::ProcType::SerialType>(1U << DpWriterNumProcPorts));
    this->ruleMaskInvalid.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
    Testers::bufferSendIn.ruleOK.apply(this->testState);
}

void Tester ::Add() {
    this->testState.setNextProcType(Fw::DpCfg::ProcType::PROC_TYPE_ZLIB_DEFLATE);
    this->ruleAdd.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
    // Adding a type already in the mask changes nothing
    this->testState.setNextProcType(Fw::DpCfg::ProcType::PROC_TYPE_ZLIB_DEFLATE);
    this->ruleAdd.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
}

void Tester ::Remove() {
    this->testState.setNextProcType(Fw::DpCfg::ProcType::PROC_TYPE_ZLIB_DEFLATE);
    this->ruleAdd.apply(this->testState);
    this->testState.setNextProcType(Fw::DpCfg::ProcType::PROC_TYPE_ONE);
    this->ruleAdd.apply(this->testState);
    this->testState.setNextProcType(Fw::DpCfg::ProcType::PROC_TYPE_ZLIB_DEFLATE);
    this->ruleRemove.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
    // Removing a type not in the mask changes nothing
    this->testState.setNextProcType(Fw::DpCfg::ProcType::PROC_TYPE_ZLIB_DEFLATE);
    this->ruleRemove.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
}

void Tester ::ForceOn() {
    this->overrideThenWrite(Fw::Enabled::ENABLED, AbstractState::getAllProcMask());
}

void Tester ::ForceOff() {
    this->overrideThenWrite(Fw::Enabled::ENABLED, 0);
}

void Tester ::Clear() {
    this->ForceOn();
    // A DISABLED override restores header-controlled processing whatever mask it carries
    this->testState.setNextProcOverrideState(Fw::Enabled::DISABLED);
    this->ruleState.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
    this->write();
}

void Tester ::AddThenEnable() {
    // Editing the mask while DISABLED does not affect processing
    this->testState.setNextProcType(Fw::DpCfg::ProcType::PROC_TYPE_ZLIB_DEFLATE);
    this->ruleAdd.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
    this->write();
    // Enabling applies the edited mask
    this->testState.setNextProcOverrideState(Fw::Enabled::ENABLED);
    this->ruleState.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
    this->write();
}

void Tester ::overrideThenWrite(Fw::Enabled::T state, Fw::DpCfg::ProcType::SerialType mask) {
    this->testState.setNextProcOverrideMask(mask);
    this->ruleMaskOK.apply(this->testState);
    this->testState.setNextProcOverrideState(state);
    this->ruleState.apply(this->testState);
    Testers::schedIn.ruleOK.apply(this->testState);
    this->write();
}

void Tester ::write() {
    Testers::bufferSendIn.ruleOK.apply(this->testState);
    Testers::bufferSendIn.ruleOKProcShrink.apply(this->testState);
}

}  // namespace ProcOverride

}  // namespace Svc
