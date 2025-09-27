// ======================================================================
// \title  FaultManager.hpp
// \author mstarch
// \brief  hpp file for FaultManager component implementation class
// ======================================================================

#ifndef Svc_FaultManager_HPP
#define Svc_FaultManager_HPP

#include "Svc/FaultProtection/FaultManager/FaultManagerComponentAc.hpp"

namespace Svc {

class FaultManager final : public FaultManagerComponentBase {
  public:
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
    void completionIn_handler(FwIndexType portNum,                           //!< The port number
                              const Fw::Success& status,                     //!< Status of the fault response
                              const FaultCfg::FaultResponse& faultResponse,  //!< Fault response enumeration
                              const FaultCfg::FaultId& faultId               //!< ID of the reported fault
                              ) override;

    //! Handler implementation for reportIn
    void reportIn_handler(FwIndexType portNum,              //!< The port number
                          const FaultCfg::FaultId& faultId  //!< ID of the reported fault
                          ) override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command TODO
    //!
    //! TODO
    void TODO_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                         U32 cmdSeq            //!< The command sequence number
                         ) override;
};

}  // namespace Svc

#endif
