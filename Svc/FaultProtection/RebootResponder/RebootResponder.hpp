// ======================================================================
// \title  RebootResponder.hpp
// \author mstarch
// \brief  hpp file for RebootResponder component implementation class
// ======================================================================

#ifndef Svc_RebootResponder_HPP
#define Svc_RebootResponder_HPP

#include "Svc/FaultProtection/RebootResponder/RebootResponderComponentAc.hpp"

namespace Svc {

class RebootResponder final : public RebootResponderComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct RebootResponder object
    RebootResponder(const char* const compName  //!< The component name
    );

    //! Destroy RebootResponder object
    ~RebootResponder();

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
