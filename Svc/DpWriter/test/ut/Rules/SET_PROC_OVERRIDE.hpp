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
};

}  // namespace SET_PROC_OVERRIDE

}  // namespace Svc

#endif
