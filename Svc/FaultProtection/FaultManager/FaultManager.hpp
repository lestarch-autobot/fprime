// ======================================================================
// \title  FaultManager.hpp
// \author mstarch
// \brief  hpp file for FaultManager component implementation class
// ======================================================================

#ifndef Svc_FaultProtection_FaultManager_HPP
#define Svc_FaultProtection_FaultManager_HPP

#include "Svc/FaultProtection/FaultManager/FaultManagerComponentAc.hpp"
#include "Fw/Prm/PrmExternalTypes.hpp"
#include "Svc/FaultProtection/FaultManager/StepDefinitionTableArrayAc.hpp"
#include "Svc/FaultProtection/FaultManager/ResponseDefinitionTableArrayAc.hpp"

namespace Svc {

namespace FaultProtection {

class FaultManager final : public FaultManagerComponentBase, public Fw::ParamExternalDelegate {
  public:
    /**
     * \struct GovernedState: State governed by the state machine actions
     */
    struct GovernedState {
        FwSizeType countdown; //!< Countdown for delayed response execution
        Fw::Success response_result; //!< Result of the response execution
        FwSizeType active_response_index; //!< Index of active response being executed
        FwSizeType active_step_index; //!< Index of the currently active step in the active response
        std::atomic<bool> latched_fault_reports[FaultResponseTable::SIZE]; //!< Array tracking which faults are currently latched
    };

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct FaultManager object
    FaultManager(const char* const compName  //!< The component name
    );

    //! Destroy FaultManager object
    ~FaultManager();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for completionIn
    //!
    //! Incoming response step completion
    void stepCompletionIn_handler(FwIndexType portNum,                    //!< The port number
                              const Fw::Success& status,              //!< Status of the fault response
                              const FaultConfig::Response& response,  //!< Active fault response
                              const FaultConfig::Step& step           //!< Step of the active fault response
                              ) override;

    //! Handler implementation for reportIn
    //!
    //! Incoming fault report
    void reportIn_handler(FwIndexType portNum,          //!< The port number
                          const FaultConfig::Fault& id  //!< ID of the reported fault
                          ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command SET_FAULT_ENABLED
    //!
    //! Set a fault enabled state
    //!
    //! Enable/disable response to the supplied Fault ID. This will update the internal parameter and may be
    //! persisted by FAULT_RESPONSE_TABLE_SAVE. Command is dropped on overflow to prevent triggering fault response.
    void SET_FAULT_ENABLED_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                      U32 cmdSeq,           //!< The command sequence number
                                      FaultConfig::Fault fault,
                                      Fw::Enabled enabled) override;

    //! Handler implementation for command SET_RESPONSE_ENABLED
    //!
    //! Set a response enabled state
    //! 
    //! Enable/disable response. This will update the internal parameter and may be persisted by
    //! RESPONSE_TABLE_SAVE. Command is dropped on overflow to prevent triggering fault response.
    void SET_RESPONSE_ENABLED_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                         U32 cmdSeq,           //!< The command sequence number
                                         FaultConfig::Response response,
                                         Fw::Enabled enabled) override;

    //! Handler implementation for command UPDATE_RESPONSE_STEP
    //!
    //! Set a response step failure mode
    //! 
    //! Set the FAILURE_MODE of response step. This will update the internal parameter and may be persisted by
    //! STEP_TABLE_SAVE. Command is dropped on overflow to prevent triggering fault response.
    void UPDATE_STEP_FAILURE_MODE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                             U32 cmdSeq,           //!< The command sequence number
                                             FaultConfig::Step step,
                                             FaultConfig::FailureMode failureMode) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined internal interfaces
    // ----------------------------------------------------------------------

    //! Handler implementation for handleReport
    //!
    //! Internal port for handling non-discarded fault report
    void handleReport_internalInterfaceHandler(const FaultConfig::Fault& fault) override;
  private:
    // ----------------------------------------------------------------------
    // Implementations for external parameter handling
    // ----------------------------------------------------------------------

    //! Deserialize a parameter from a parameter buffer
    //!
    //! \param base_id: The component base ID of the parameter being deserialized
    //! \param local_id: The local parameter ID of the parameter being deserialized
    //! \param prmStat: The parameter status of the parameter being deserialized
    //! \param buff: The buffer contained the serialized parameter
    //!
    //! \return: The status of the deserialize operation
     Fw::SerializeStatus deserializeParam(const FwPrmIdType base_id,
                                          const FwPrmIdType local_id,
                                          const Fw::ParamValid prmStat,
                                          Fw::SerialBufferBase& buff) override;

    //! Serialize a parameter into a parameter buffer
    //!
    //! \param base_id: The component base ID of the parameter being deserialized
    //! \param local_id: The local Parameter ID of the parameter to serialized
    //! \param buff: The buffer to serialize the parameter into
    //!
    //! \return: The status of the serialize operation
    Fw::SerializeStatus serializeParam(const FwPrmIdType base_id,
                                       const FwPrmIdType local_id,
                                       Fw::SerialBufferBase& buff) const override;
  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action startCountdown of state machine Svc_FaultProtection_FaultManagerStateMachine
    //!
    //! Action to start countdown
    void Svc_FaultProtection_FaultManagerStateMachine_action_startCountdown(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action decrementCountdown of state machine Svc_FaultProtection_FaultManagerStateMachine
    //!
    //! Action to decrement countdown
    void Svc_FaultProtection_FaultManagerStateMachine_action_decrementCountdown(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action selectResponse of state machine Svc_FaultProtection_FaultManagerStateMachine
    //!
    //! Action to select response to execute
    void Svc_FaultProtection_FaultManagerStateMachine_action_selectResponse(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action completeResponse of state machine Svc_FaultProtection_FaultManagerStateMachine
    //!
    //! Action to select response to execute
    void Svc_FaultProtection_FaultManagerStateMachine_action_completeResponse(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action dispatchStep of state machine Svc_FaultProtection_FaultManagerStateMachine
    //!
    //! Action to dispatch a response step
    void Svc_FaultProtection_FaultManagerStateMachine_action_dispatchStep(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action setResponseFailure of state machine Svc_FaultProtection_FaultManagerStateMachine
    //!
    //! Action to set response failure
    void Svc_FaultProtection_FaultManagerStateMachine_action_setResponseFailure(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
        ) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine guards
    // ----------------------------------------------------------------------

    //! Implementation for guard hasReport of state machine Svc_FaultProtection_FaultManagerStateMachine
    //!
    //! Check if there is a fault report
    bool Svc_FaultProtection_FaultManagerStateMachine_guard_hasReport(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
    ) const override;

    //! Implementation for guard countdownExpired of state machine Svc_FaultProtection_FaultManagerStateMachine
    //!
    //! Check if countdown has expired
    bool Svc_FaultProtection_FaultManagerStateMachine_guard_countdownExpired(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
    ) const override;

    //! Implementation for guard responseDone of state machine Svc_FaultProtection_FaultManagerStateMachine
    //!
    //! Check if response is done executing each step
    bool Svc_FaultProtection_FaultManagerStateMachine_guard_responseDone(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
    ) const override;

  private:
    // ----------------------------------------------------------------------
    // Private helper functions
    // ----------------------------------------------------------------------

    const StepDefinitionEntry& stepToStepEntry(const FaultConfig::Step& step);

    FwSizeType responseToResponseEntryIndex(const FaultConfig::Response& response);

    void dispatchStep(const FaultConfig::Response& response, const FaultConfig::Step& step);
  private:
      FaultResponseTable m_fault_parameter;
      ResponsesEnabled m_response_parameter;
      StepFailureModes m_step_parameter;

      // Step definition table
      const ResponseDefinitionTable m_response_definition_table;
      const StepDefinitionTable m_step_definition_table;


      GovernedState m_sm_state; //!< State governed by the state machine actions
};

}  // namespace FaultProtection
}  // namespace Svc

#endif
