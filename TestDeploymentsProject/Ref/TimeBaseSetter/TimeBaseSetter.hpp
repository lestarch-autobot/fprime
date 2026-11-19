// ======================================================================
// \title  TimeBaseSetter.hpp
// \brief  Test component passing time from a time source, overriding its time base on command
// ======================================================================

#ifndef Ref_TimeBaseSetter_HPP
#define Ref_TimeBaseSetter_HPP

#include <atomic>

#include "Ref/TimeBaseSetter/TimeBaseSetterComponentAc.hpp"

namespace Ref {

class TimeBaseSetter final : public TimeBaseSetterComponentBase {
  public:
    //! Construct TimeBaseSetter object
    explicit TimeBaseSetter(const char* const compName  //!< The component name
    );

    //! Destroy TimeBaseSetter object
    ~TimeBaseSetter();

  private:
    //! Value of m_override meaning no override
    static constexpr U32 NO_OVERRIDE = 0xFFFFFFFFU;

    //! Handler implementation for timeGetPort: the time of the time source, with the override time base when set
    void timeGetPort_handler(FwIndexType portNum,  //!< The port number
                             Fw::Time& time        //!< Reference to Time object
                             ) override;

    //! Handler implementation for command SET_TIME_BASE
    void SET_TIME_BASE_cmdHandler(FwOpcodeType opCode,      //!< The opcode
                                  U32 cmdSeq,               //!< The command sequence number
                                  const TimeBase& timeBase  //!< Time base carried by every answer
                                  ) override;

    //! Handler implementation for command CLEAR_TIME_BASE
    void CLEAR_TIME_BASE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                    U32 cmdSeq            //!< The command sequence number
                                    ) override;

    //! Override time base, or NO_OVERRIDE; read on the thread of every time caller
    std::atomic<U32> m_override;
};

}  // namespace Ref

#endif
