module Ref {

  @ Test component: passes time from the attached time source, overriding its time base on command
  passive component TimeBaseSetter {

    @ Time request
    sync input port timeGetPort: Fw.Time

    @ Time request to the attached time source
    output port timeSourceOut: Fw.Time

    @ Set the override time base
    sync command SET_TIME_BASE(
      timeBase: TimeBase @< Time base carried by every answer
    )

    @ Clear the override
    sync command CLEAR_TIME_BASE

    @ Override set
    event TimeBaseSet(timeBase: TimeBase) \
      severity activity high format "Time base override set to {}"

    @ Override cleared
    event TimeBaseCleared() \
      severity activity high format "Time base override cleared"

    time get port timeCaller
    command reg port cmdRegOut
    command recv port cmdIn
    command resp port cmdResponseOut
    text event port logTextOut
    event port logOut
  }
}
