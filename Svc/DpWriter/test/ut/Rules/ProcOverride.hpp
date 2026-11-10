// ======================================================================
// \title  ProcOverride.hpp
// \author lestarch-autobot
// \brief  ProcOverride class interface
// ======================================================================

#ifndef Svc_ProcOverride_HPP
#define Svc_ProcOverride_HPP

#include "Svc/DpWriter/test/ut/Rules/Rules.hpp"
#include "Svc/DpWriter/test/ut/TestState/TestState.hpp"

namespace Svc {

namespace ProcOverride {

class Tester {
  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! Set the override state and check the response, event, and update-on-change telemetry
    void State();

    //! Set a valid override mask directly
    void MaskOK();

    //! Reject a mask selecting a processing type with no port
    void MaskInvalid();

    //! A rejected mask leaves the current mask and state in force
    void MaskInvalidKeepsMask();

    //! Add a processing type to the mask
    void Add();

    //! Remove a processing type from the mask
    void Remove();

    //! Enable an override selecting every processing port
    void ForceOn();

    //! Enable an empty override, forcing all processing off
    void ForceOff();

    //! Disable the override and return to header-controlled processing
    void Clear();

    //! Edit the mask while disabled (not applied), then enable it
    void AddThenEnable();

  public:
    // ----------------------------------------------------------------------
    // Rules
    // ----------------------------------------------------------------------

    //! Rule ProcOverride::State
    Rules::ProcOverride::State ruleState;

    //! Rule ProcOverride::MaskOK
    Rules::ProcOverride::MaskOK ruleMaskOK;

    //! Rule ProcOverride::MaskInvalid
    Rules::ProcOverride::MaskInvalid ruleMaskInvalid;

    //! Rule ProcOverride::Add
    Rules::ProcOverride::Add ruleAdd;

    //! Rule ProcOverride::Remove
    Rules::ProcOverride::Remove ruleRemove;

  public:
    // ----------------------------------------------------------------------
    // Public member variables
    // ----------------------------------------------------------------------

    //! Test state
    TestState testState;

  private:
    //! Set the mask and state, then write a container with and without requested processing
    void overrideThenWrite(Fw::Enabled::T state,                      //!< The override state
                           Fw::DpCfg::ProcType::SerialType procTypes  //!< The override mask
    );

    //! Write a container with and without requested processing under the current override
    void write();
};

}  // namespace ProcOverride

}  // namespace Svc

#endif
