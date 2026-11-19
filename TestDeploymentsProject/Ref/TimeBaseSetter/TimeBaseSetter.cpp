// ======================================================================
// \title  TimeBaseSetter.cpp
// \brief  Test component passing time from a time source, overriding its time base on command
// ======================================================================

#include "Ref/TimeBaseSetter/TimeBaseSetter.hpp"

namespace Ref {

TimeBaseSetter::TimeBaseSetter(const char* const compName)
    : TimeBaseSetterComponentBase(compName), m_override(NO_OVERRIDE) {}

TimeBaseSetter::~TimeBaseSetter() {}

void TimeBaseSetter::timeGetPort_handler(FwIndexType portNum, Fw::Time& time) {
    this->timeSourceOut_out(0, time);
    const U32 override = this->m_override.load();
    if (override != NO_OVERRIDE) {
        const TimeBase base = static_cast<TimeBase::T>(override);
        time.set(base, time.getContext(), time.getSeconds(), time.getUSeconds());
    }
}

void TimeBaseSetter::SET_TIME_BASE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const TimeBase& timeBase) {
    this->m_override.store(static_cast<U32>(timeBase.e));
    this->log_ACTIVITY_HI_TimeBaseSet(timeBase);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void TimeBaseSetter::CLEAR_TIME_BASE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->m_override.store(NO_OVERRIDE);
    this->log_ACTIVITY_HI_TimeBaseCleared();
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Ref
