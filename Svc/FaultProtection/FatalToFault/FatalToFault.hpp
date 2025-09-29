// ======================================================================
// \title  FatalToFault.hpp
// \author mstarch
// \brief  hpp file for FatalToFault component implementation class
// ======================================================================

#ifndef Svc_FatalToFault_HPP
#define Svc_FatalToFault_HPP

#include "Svc/FaultProtection/FatalToFault/FatalToFaultComponentAc.hpp"

namespace Svc {

class FatalToFault final : public FatalToFaultComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct FatalToFault object
    FatalToFault(const char* const compName  //!< The component name
    );

    //! Destroy FatalToFault object
    ~FatalToFault();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for FatalReceive
    //!
    //! FATAL event receive port
    void FatalReceive_handler(FwIndexType portNum,  //!< The port number
                              FwEventIdType Id      //!< The ID of the FATAL event
                              ) override;
};

}  // namespace Svc

#endif
