# Ref::TimeBaseOverride

## Introduction

`Ref::TimeBaseOverride` is a test component placed between the `Ref` topology and its time
source. Every time request is forwarded to the delegate time component (`Svc::PosixTime` in
`Ref`); when an override is commanded the time base of the returned time is replaced and the
seconds and microseconds are passed through unchanged, so the delegate clock stays under
test while the topology sees a different time base.

## Requirements

| ID | Requirement | Verification |
|---|---|---|
| REQ-TBO-001 | The component shall return the delegate's time unchanged when no override is active. | Unit Test |
| REQ-TBO-002 | The component shall set the time base of every returned time to the override set by `SET_TIME_BASE`. | Unit Test |
| REQ-TBO-003 | `CLEAR_OVERRIDE` shall clear the override, restoring the delegate's time base. | Unit Test |
| REQ-TBO-004 | The component shall report each override change as an `ACTIVITY_HI` event naming both time bases. | Unit Test |
| REQ-TBO-005 | The component shall report the override time base, or `TB_DONT_CARE` when none is active, as telemetry. | Unit Test |
| REQ-TBO-006 | The component shall respond `VALIDATION_ERROR` with a `WARNING_LO` event to `SET_TIME_BASE` of `TB_DONT_CARE`. | Unit Test |

Table 1 — Commands: `SET_TIME_BASE(timeBase: TimeBase)` sets the override; `CLEAR_OVERRIDE` removes it.

## Design

```
 timeGetPort : Fw.Time  +------------------------+  timeDelegate : Fw.Time
   [sync] ------------->|  Ref.TimeBaseOverride  |------>  (Svc.PosixTime.timeGetPort)
                        |       (passive)        |
                        +------------------------+
   special ports: command, event, text event, time, telemetry
```

| Port | Kind | Type | Purpose |
|---|---|---|---|
| `timeGetPort` | sync input | `Fw.Time` | Topology time requests; mutex-free like `Svc.PosixTime` |
| `timeDelegate` | output | `Fw.Time` | Forward to the real clock |

Commands `SET_TIME_BASE(timeBase)` / `CLEAR_OVERRIDE` (sync); events `OverrideChanged(old,new)`
`ACTIVITY_HI`, `InvalidTimeBase(timeBase)` `WARNING_LO`; telemetry `OverrideTimeBase: TimeBase`.
No parameters.

### Configure

```fpp
instance timeBaseOverride: Ref.TimeBaseOverride base id 0x10027000
time connections instance timeBaseOverride          # replaces `instance posixTime`; self-connection is harmless (sync)
timeBaseOverride.timeDelegate -> posixTime.timeGetPort
```

### Behavior

`timeGetPort` calls `timeDelegate`, then, if an override is set, calls `setTimeBase(override)`
on the returned time and returns it; seconds, microseconds and context are untouched, so
`Svc.PosixTime` remains the clock under test. The override is a single
`FwTimeBaseStoreType` (16-bit) written by the command handlers on the dispatcher thread and read by
every caller of `getTime()`; it is held as `std::atomic<FwTimeBaseStoreType>` with relaxed ordering
(a `volatile` aligned 16-bit store if the project's C++ subset excludes `<atomic>`), so no mutex is
taken on the time path and no data race exists at the language level. `SET_TIME_BASE` rejects
`TB_DONT_CARE` (`VALIDATION_ERROR`, `InvalidTimeBase`); otherwise it stores the override, emits
`OverrideChanged` and telemetry. `CLEAR_OVERRIDE` stores `TB_DONT_CARE` meaning "none". The
component's own event time stamps go through the `time` pattern to itself and are overridden too.

Out-of-scope failure modes: the delegate clock's accuracy and base (`Svc.PosixTime`).

## Unit-test list

| Test | Description | Requirements |
|---|---|---|
| `PassThrough` | With no override the delegate's time returns unchanged. | REQ-TBO-001 |
| `Override` | After `SET_TIME_BASE` every returned time carries the override; seconds/useconds untouched. | REQ-TBO-002, -004 |
| `Clear` | `CLEAR_OVERRIDE` restores the delegate's base with `OverrideChanged`. | REQ-TBO-003, -004 |
| `Telemetry` | `OverrideTimeBase` reports the override or `TB_DONT_CARE`. | REQ-TBO-005 |
| `RejectDontCare` | `SET_TIME_BASE(TB_DONT_CARE)` returns `VALIDATION_ERROR` with `InvalidTimeBase`. | REQ-TBO-006 |
