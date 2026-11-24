// ======================================================================
// \title  Interceptors.cpp
// \author Canham/Bocchino
// \brief  Implementation for CmdSequencerTester::Interceptors
//
// \copyright
// Copyright (C) 2009-2018 California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
// ======================================================================

#include "CmdSequencerTester.hpp"
#include "gtest/gtest.h"

namespace Svc {

CmdSequencerTester::Interceptor::Interceptor()
    : enabled(EnableType::NONE),
      errorType(ErrorType::NONE),
      waitCount(0),
      size(0),
      fileStatus(Os::File::OP_OK),
      fileInterceptor(*this) {}

CmdSequencerTester::Interceptor::FileInterceptor::FileInterceptor(Interceptor& interceptor)
    : m_interceptor(interceptor) {}

void CmdSequencerTester::Interceptor::enable(EnableType::t enableType) {
    this->enabled = enableType;
}

void CmdSequencerTester::Interceptor::disable() {
    this->enabled = EnableType::t::NONE;
}

Os::File::Status CmdSequencerTester::Interceptor::FileInterceptor::open(Os::FileInterface& real,
                                                                        const char* path,
                                                                        Mode mode,
                                                                        OverwriteType overwrite) {
    if (this->m_interceptor.enabled == EnableType::t::OPEN) {
        return this->m_interceptor.fileStatus;
    }
    return real.open(path, mode, overwrite);
}

Os::File::Status CmdSequencerTester::Interceptor::FileInterceptor::read(Os::FileInterface& real,
                                                                        U8* buffer,
                                                                        FwSizeType& requestSize,
                                                                        Os::File::WaitType waitType) {
    Os::File::Status status = real.read(buffer, requestSize, waitType);
    if ((this->m_interceptor.enabled == EnableType::READ) && (this->m_interceptor.errorType != ErrorType::NONE)) {
        if (this->m_interceptor.waitCount > 0) {
            // Not time to inject an error yet: decrement wait count
            --this->m_interceptor.waitCount;
        } else {
            // Time to inject an error: check test scenario
            switch (this->m_interceptor.errorType) {
                case ErrorType::READ:
                    status = this->m_interceptor.fileStatus;
                    break;
                case ErrorType::SIZE:
                    requestSize = this->m_interceptor.size;
                    status = Os::File::OP_OK;
                    break;
                case ErrorType::DATA:
                    memcpy(buffer, this->m_interceptor.data, this->m_interceptor.size);
                    status = Os::File::OP_OK;
                    break;
                default:
                    EXPECT_TRUE(false);
                    break;
            }
        }
    }
    return status;
}
}  // namespace Svc
