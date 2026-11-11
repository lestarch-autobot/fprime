module Ref {

  @ Test helper: forwards time requests to a delegate and overrides the time base on command
  passive component TimeBaseOverride {

    @ Time requests from the topology
    sync input port timeGetPort: Fw.Time

    @ Time requests forwarded to the delegate time component
    output port timeDelegate: Fw.Time

    command recv port cmdIn
    command reg port cmdRegOut
    command resp port cmdResponseOut
    event port eventOut
    text event port textEventOut
    time get port timeGetOut
    telemetry port tlmOut

    @ Override the time base of every returned time
    sync command SET_TIME_BASE(timeBase: TimeBase) opcode 0

    @ Remove the override; the delegate's time base is returned again
    sync command CLEAR_OVERRIDE opcode 1

    @ The override changed (TB_DONT_CARE = no override)
    event OverrideChanged(previous: TimeBase, current: TimeBase) severity activity high id 0 \
      format "Time base override changed from {} to {}"

    @ SET_TIME_BASE rejected
    event InvalidTimeBase(timeBase: TimeBase) severity warning low id 1 \
      format "Time base {} cannot be an override"

    @ Active override; TB_DONT_CARE when none
    telemetry OverrideTimeBase: TimeBase id 0
  }
}
