# Ref topology delta — SeqScheduler and TimeBaseOverride

Instances (`Top/instances.fpp`; base IDs 0x10026000/0x10027000 are unused in Ref):

```fpp
instance seqScheduler: Svc.SeqScheduler base id 0x10026000 \
  queue size 32                       # >= 2 * SeqSchedulerRows + 4 (SDD Configure); Default.QUEUE_SIZE (10) is too small
instance timeBaseOverride: Ref.TimeBaseOverride base id 0x10027000
```

Connections (`Top/topology.fpp`):

```fpp
instance seqScheduler
instance timeBaseOverride
time connections instance timeBaseOverride            # was: instance posixTime
connections RateGroups {
  rateGroup1Comp.RateGroupMemberOut[8] -> seqScheduler.run     # 1 Hz; slot 8 free (ActiveRateGroupOutputPorts = 10)
}
connections SeqScheduler {
  seqScheduler.seqRunOut    -> cmdSeq.seqRunIn
  seqScheduler.seqCancelOut -> cmdSeq.seqCancelIn
  cmdSeq.seqStartOut        -> seqScheduler.seqStartIn
  cmdSeq.seqDone            -> seqScheduler.seqDoneIn
  seqScheduler.productGetOut  -> DataProducts.Subtopology.productGetIn
  seqScheduler.productSendOut -> DataProducts.Subtopology.productSendIn
}
connections TimeBaseOverride {
  timeBaseOverride.timeDelegate -> posixTime.timeGetPort
  # no explicit timeGetOut wiring: the `time connections` pattern connects every instance,
  # timeBaseOverride included (fpp-check: a second connection is "too many ports connected"),
  # and the port is sync so the self-connection is harmless
}
```

Telemetry packets (`Top/RefPackets.fppi`): add a `SeqScheduler` packet (level 1) carrying all ten
`seqScheduler` channels and `timeBaseOverride.OverrideTimeBase` — fpp-check requires every channel packed
or omitted, and the system tests read `SinceStart`.

Configuration (`Top/RefTopology.cpp`): `seqScheduler.configure(Fw::TimeInterval(1, 0))` — the rate-group-1
period, so relative offsets and tick parameters are seconds in Ref.

`cmdSeq.seqDone`, `seqStartOut` and `seqRunIn`/`seqCancelIn` are unconnected in Ref today (only `schedIn`,
`comCmdOut`, `cmdResponseIn` are wired), so no existing consumer is displaced. Both components
are registered with `add_fprime_subdirectory` (`Svc/CMakeLists.txt`, `Ref/CMakeLists.txt`).
`Svc.PosixTime` keeps reporting `TB_WORKSTATION_TIME`; the override lets a test switch the
topology to `TB_SC_TIME` (or any base) on command, which is what `TIME_BASE_RELATIVE` and
`ABSOLUTE` rows key on. No health ping (queued component, no thread). The 1 Hz slot gives
`BASE_WAIT_TICKS`/`DONE_TIMEOUT_TICKS` one-second units in Ref.
