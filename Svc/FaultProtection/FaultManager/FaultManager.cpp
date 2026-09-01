// ======================================================================
// \title  FaultManager.cpp
// \author mstarch
// \brief  cpp file for FaultManager component implementation class
// ======================================================================

#include "Svc/FaultProtection/FaultManager/FaultManager.hpp"
#include "Fw/Types/Assert.hpp"
#include "Fw/Logger/Logger.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

FaultManager ::FaultManager(const char* const compName) {
    this->registerExternalParameters(this);
}

FaultManager ::~FaultManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void FaultManager ::reportIn_handler(FwIndexType portNum, const FaultConfig::Fault& id) {
    // TODO: find fault report and latch it!

    /*if (not fault) {
        startResponse();
    } else if (lower precedence) {
        cancelResponse();
    } else {
        ignoreResponse();
    }*/
}

void FaultManager ::stepCompletionIn_handler(FwIndexType portNum,
                                             const Fw::Success& status,
                                             const FaultConfig::Response& response,
                                             const FaultConfig::Step& step) {
    StepDefinitionEntry step_entry = this->stepToStepEntry(step);
    if (status == Fw::Success::FAILURE) {
        FaultConfig::FailureMode::T failureMode = step_entry.get_failureMode();
        switch (failureMode) {
            // IGNORE will continue as if the failure was a success
            case FaultConfig::FailureMode::IGNORE:
                this->faultManagerStateMachine_sendSignal_StepSuccessful();
                break;
            // DEFER will continue the response but treat the response as a failure
            case FaultConfig::FailureMode::DEFER:
                this->faultManagerStateMachine_sendSignal_StepDeferredFailure();
                break;
            // FAULT will stop response execution and treat the response as a failure
            case FaultConfig::FailureMode::FAULT:
                this->faultManagerStateMachine_sendSignal_StepFailed();
                break;
        }
    } else {
        this->faultManagerStateMachine_sendSignal_StepSuccessful();
    }
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void FaultManager ::SET_FAULT_ENABLED_cmdHandler(FwOpcodeType opCode,
                                                 U32 cmdSeq,
                                                 FaultConfig::Fault fault,
                                                 Fw::Enabled enabled) {
    FW_ASSERT(fault < FaultConfig::Fault::NUM_FAULTS, static_cast<FwAssertArgType>(fault));
    this->m_fault_parameter[fault].set_enabled(enabled);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void FaultManager ::SET_RESPONSE_ENABLED_cmdHandler(FwOpcodeType opCode,
                                                    U32 cmdSeq,
                                                    FaultConfig::Response response,
                                                    Fw::Enabled enabled) {
    FW_ASSERT(response < FaultConfig::Response::NUM_RESPONSES, static_cast<FwAssertArgType>(response));
    this->m_response_parameter[response] = enabled;
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void FaultManager ::UPDATE_STEP_FAILURE_MODE_cmdHandler(FwOpcodeType opCode,
                                                        U32 cmdSeq,
                                                        FaultConfig::Step step,
                                                        FaultConfig::FailureMode failureMode) {
    FW_ASSERT(step < FaultConfig::Step::NUM_STEPS, static_cast<FwAssertArgType>(step));
    this->m_step_parameter[step] = failureMode;
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined internal interfaces
// ----------------------------------------------------------------------

void FaultManager ::handleReport_internalInterfaceHandler(const FaultConfig::Fault& fault) {
    // TODO
}

// ----------------------------------------------------------------------
// Implementations for helper functions
// ----------------------------------------------------------------------

const StepDefinitionEntry& FaultManager ::stepToStepEntry(const FaultConfig::Step& step) {
    for (FwSizeType i = 0; i < StepDefinitionTable::SIZE; i++) {
        if (this->m_step_definition_table[i].get_step() == step) {
            return this->m_step_definition_table[i];
        }
    }
    FW_ASSERT(0, static_cast<FwAssertArgType>(step));
    //TODO: what to do here?
    return this->m_step_definition_table[0];
}

FwSizeType FaultManager ::responseToResponseEntryIndex(const FaultConfig::Response& response) {
    for (FwSizeType i = 0; i < ResponseDefinitionTable::SIZE; i++) {
        if (this->m_response_definition_table[i].get_response() == response) {
            return i;
        }
    }
    FW_ASSERT(0, static_cast<FwAssertArgType>(response));
    //TODO: what to do here?
    return 0;
}

void FaultManager ::dispatchStep(const FaultConfig::Response& response, const FaultConfig::Step& step) {
    const StepDefinitionEntry& step_entry = this->stepToStepEntry(step);

    // Since this is fault management, we must be very careful not to trigger fault responses w.r.t. handling faults.
    if (this->isConnected_stepDispatchOut_OutputPort(step_entry.get_dispatchPort())) {
        this->stepDispatchOut_out(step_entry.get_dispatchPort(), response, step, step_entry.get_context());
    }
    // When this should have asserted, emit an ERROR and move on
    else {
        Fw::Logger::log("[CRITICAL] FaultManager: Attempted to dispatch step on non-connected port %d. Response: %d, Step: %d",
                         step_entry.get_dispatchPort(), response.e, step.e);
        Fw::Logger::log("        This is a configuration error. Step will be considered FAILED and execution will continue.");
        // Trigger the step complete handler immediately with a failure status to indicate this step did not execute.
        this->stepCompletionIn_handler(std::numeric_limits<FwIndexType>::max(), Fw::Success::FAILURE, response, step);
    }
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_action_startCountdown(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) {
    this->m_sm_state.countdown = 4; // TODO: this should be driven from configuration
}

void FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_action_decrementCountdown(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) {
    // Decrement countdown with underflow protection
    this->m_sm_state.countdown = (this->m_sm_state.countdown > 0) ? this->m_sm_state.countdown - 1 : 0;
}

void FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_action_selectResponse(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) {
    
    bool found_some_fault = false;
    U8 current_precedence = 0;

    for (FwSizeType i = 0; i < FaultResponseTable::SIZE; i++) {
        if (this->m_sm_state.latched_fault_reports[i] && this->m_fault_parameter[i].get_enabled() == Fw::Enabled::ENABLED) {
            const U8 fault_precedence = this->m_fault_parameter[i].get_precedence();
            if (not found_some_fault || (fault_precedence > current_precedence)) {
                current_precedence = fault_precedence;
                this->m_sm_state.active_response_index = this->responseToResponseEntryIndex(this->m_fault_parameter[i].get_response());
                found_some_fault = true;
            }
        }
    }
    // Since we are selecting a response, then some response must be active
    FW_ASSERT(found_some_fault);

    // TODO: emit starting fault response event

    // Fault response state always starts as successful and is driven to failure in specific conditions
    this->m_sm_state.response_result = Fw::Success::SUCCESS;
}

void FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_action_completeResponse(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) {
    
    if (this->m_sm_state.response_result == Fw::Success::SUCCESS) {
        //TODO: clear latched faults
        //TODO: emit completed fault response failure event
    } else {
        //TODO: emit completed fault response failure event
        //TODO: clear latched faults when clear-on-failure
    }

    // Reset state related to active response
    this->m_sm_state.active_response_index = std::numeric_limits<FwSizeType>::max();
    this->m_sm_state.response_result = Fw::Success::SUCCESS;
}

void FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_action_dispatchStep(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) {
    FW_ASSERT(this->m_sm_state.active_response_index < ResponseDefinitionTable::SIZE, static_cast<FwAssertArgType>(this->m_sm_state.active_response_index));
    const ResponseDefinitionEntry& response_entry = this->m_response_definition_table[this->m_sm_state.active_response_index];
    this->dispatchStep(response_entry.get_response(), response_entry.get_steps()[this->m_sm_state.active_step_index]);
    this->m_sm_state.active_step_index++;
}

void FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_action_setResponseFailure(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) {
    // Response failed and thus latches a failure
    this->m_sm_state.response_result = Fw::Success::FAILURE;
}

// ----------------------------------------------------------------------
// Implementations for internal state machine guards
// ----------------------------------------------------------------------

bool FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_guard_hasReport(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) const {
    // Check all latched faults to see if there is an active report and that the response is enabled
    for (FwSizeType i = 0; i < FaultResponseTable::SIZE; i++) {
        if (this->m_sm_state.latched_fault_reports[i] && this->m_fault_parameter[i].get_enabled() == Fw::Enabled::ENABLED) {
            return true;
        }
    }
    return false;
}

bool FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_guard_countdownExpired(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) const {
    return this->m_sm_state.countdown == 0;
}

bool FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_guard_responseDone(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) const {
    FW_ASSERT(this->m_sm_state.active_response_index < ResponseDefinitionTable::SIZE, static_cast<FwAssertArgType>(this->m_sm_state.active_response_index));
    const ResponseDefinitionEntry& response_entry = this->m_response_definition_table[this->m_sm_state.active_response_index];
    // Response is done is done when the step index is out of bounds ...
    return (this->m_sm_state.active_step_index >= Steps::SIZE) ||
        // ... or when the current step is a SKIP step
        (response_entry.get_steps()[this->m_sm_state.active_step_index].e == FaultConfig::Step::SKIP);
}


// ----------------------------------------------------------------------
// Implementations for external parameter handling
// ----------------------------------------------------------------------

// TODO: should this be moved into a helper?
Fw::SerializeStatus FaultManager ::deserializeParam(const FwPrmIdType base_id, const FwPrmIdType local_id, const Fw::ParamValid prmStat, Fw::SerialBufferBase& buff) {
    // TODO: validate the tables are correct before allowing them to be set
    Fw::SerializeStatus status = Fw::SerializeStatus::FW_DESERIALIZE_FORMAT_ERROR;
    switch (base_id) {
        case PARAMID_FAULT_RESPONSE_TABLE:
            status = m_fault_parameter.deserializeFrom(buff);
            break;
        case PARAMID_RESPONSE_TABLE:
            status = m_response_parameter.deserializeFrom(buff);
            break;
        case PARAMID_STEP_TABLE:
            status = m_step_parameter.deserializeFrom(buff);
            break;
        default:
            FW_ASSERT(0, static_cast<FwAssertArgType>(base_id));
            break;
    }
    return status;
}

// TODO: should this be moved into a helper?
Fw::SerializeStatus FaultManager ::serializeParam(const FwPrmIdType base_id, const FwPrmIdType local_id, Fw::SerialBufferBase& buff) const {
    Fw::SerializeStatus status = Fw::SerializeStatus::FW_SERIALIZE_FORMAT_ERROR;
    switch (base_id) {
        case PARAMID_FAULT_RESPONSE_TABLE:
            status = m_fault_parameter.serializeTo(buff);
            break;
        case PARAMID_RESPONSE_TABLE:
            status = m_response_parameter.serializeTo(buff);
            break;
        case PARAMID_STEP_TABLE:
            status = m_step_parameter.serializeTo(buff);
            break;
        default:
            FW_ASSERT(0, static_cast<FwAssertArgType>(base_id));
            break;
    }
    return status;
}

}  // namespace FaultProtection

}  // namespace Svc
