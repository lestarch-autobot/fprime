// ======================================================================
// \title  SequenceResponder.hpp
// \author mstarch
// \brief  hpp file for SequenceResponder component implementation class
// ======================================================================

#ifndef Svc_FaultProtection_SequenceResponder_HPP
#define Svc_FaultProtection_SequenceResponder_HPP

#include "Svc/FaultProtection/SequenceResponder/SequenceResponderComponentAc.hpp"

namespace Svc {

namespace FaultProtection {

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

    //! Handler implementation for faultResponseCancel
    //!
    //! Cancel a running fault response step
    void faultResponseCancel_handler(FwIndexType portNum  //!< The port number
                                     ) override;

    //! Handler implementation for faultResponseDispatch
    //!
    //! Start a fault response step
    void faultResponseDispatch_handler(
        FwIndexType portNum,                    //!< The port number
        const FaultConfig::Response& response,  //!< Active fault response
        const FaultConfig::Step& step,          //!< Step of the active fault response
        const FaultConfig::Context& context     //!< Context of the step of the active fault response
        ) override;
};

}  // namespace FaultProtection

}  // namespace Svc

#endif
