// ======================================================================
// \title  SequenceResponder.hpp
// \author mstarch
// \brief  hpp file for SequenceResponder component implementation class
// ======================================================================

#ifndef Svc_SequenceResponder_HPP
#define Svc_SequenceResponder_HPP

#include "Svc/FaultProtection/SequenceResponder/SequenceResponderComponentAc.hpp"

namespace Svc {

class SequenceResponder final : public SequenceResponderComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct SequenceResponder object
    SequenceResponder(const char* const compName  //!< The component name
    );

    //! Destroy SequenceResponder object
    ~SequenceResponder();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for responseDispatch
    //!
    //! Incoming fault response dispatches
    Fw::Success responseDispatch_handler(FwIndexType portNum,                           //!< The port number
                                         const FaultCfg::FaultResponse& faultResponse,  //!< Fault response enumeration
                                         const FaultCfg::FaultId& faultId               //!< ID of the reported fault
                                         ) override;
};

}  // namespace Svc

#endif
