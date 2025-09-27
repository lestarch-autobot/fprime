// ======================================================================
// \title  FaultManager.cpp
// \author mstarch
// \brief  cpp file for FaultManager component implementation class
// ======================================================================

#include "Svc/FaultProtection/FaultManager/FaultManager.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

FaultManager ::FaultManager(const char* const compName) : FaultManagerComponentBase(compName) {}

FaultManager ::~FaultManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void FaultManager ::completionIn_handler(FwIndexType portNum,
                                         const Fw::Success& status,
                                         const FaultCfg::FaultResponse& faultResponse,
                                         const FaultCfg::FaultId& faultId) {
    // TODO
}

void FaultManager ::reportIn_handler(FwIndexType portNum, const FaultCfg::FaultId& faultId) {
    // TODO
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void FaultManager ::TODO_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Svc
