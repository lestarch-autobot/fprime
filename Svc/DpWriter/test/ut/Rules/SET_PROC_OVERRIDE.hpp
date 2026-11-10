// ======================================================================
// \title  SET_PROC_OVERRIDE.hpp
// \author lestarch-autobot
// \brief  SET_PROC_OVERRIDE class interface
// ======================================================================

#ifndef Svc_SET_PROC_OVERRIDE_HPP
#define Svc_SET_PROC_OVERRIDE_HPP

#include "Svc/DpWriter/test/ut/Rules/Rules.hpp"
#include "Svc/DpWriter/test/ut/TestState/TestState.hpp"

namespace Svc {

namespace SET_PROC_OVERRIDE {

class Tester {
  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! OK
    void OK();

    //! Invalid
    void Invalid();

    //! Reject forcing on the existing processing port that is not connected
    void InvalidUnconnected();

    //! Force processing on for every connected processing port
    void ForceOn();

    //! Force processing off for every processing port
    void ForceOff();

    //! Clear the override and return to header-controlled processing
    void Clear();

  public:
    // ----------------------------------------------------------------------
    // Rules
    // ----------------------------------------------------------------------

    //! Rule SET_PROC_OVERRIDE::Invalid
    Rules::SET_PROC_OVERRIDE::Invalid ruleInvalid;

    //! Rule SET_PROC_OVERRIDE::OK
    Rules::SET_PROC_OVERRIDE::OK ruleOK;

  public:
    // ----------------------------------------------------------------------
    // Public member variables
    // ----------------------------------------------------------------------

    //! Test state
    TestState testState;

  private:
    //! Set the override, then write a container with and without requested processing
    void overrideThenWrite(Fw::Enabled::T state,                      //!< The override state
                           Fw::DpCfg::ProcType::SerialType procTypes  //!< The override mask
    );
};

}  // namespace SET_PROC_OVERRIDE

}  // namespace Svc

#endif
