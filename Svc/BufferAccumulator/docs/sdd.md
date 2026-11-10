# Svc::BufferAccumulator

`Svc::BufferAccumulator` is an active component that accepts `Fw::Buffer` objects on a fill port, holds them in a bounded FIFO, and releases them on a drain port either continuously (DRAIN mode) or a commanded number at a time (`BA_DrainBuffers`). Each released buffer is expected back on a return port, after which it is forwarded to its owner. The **drain limit** bounds the number of released buffers that may be outstanding (sent and not yet returned) at once; the default limit of 1 gives the historical one-buffer-in-flight behavior, and a larger limit pipelines buffers to the downstream consumer (nasa/fprime issue #6071).

## 1. Requirements

| ID | Requirement | Verification |
|---|---|---|
| REQ-BA-DL-001 | When the queue-initialization call supplies no initial drain limit, the drain limit after initialization shall be 1. | Unit test |
| REQ-BA-DL-002 | When the queue-initialization call supplies an initial drain limit L with 1 ≤ L ≤ queue depth, the drain limit after initialization shall equal L. | Unit test |
| REQ-BA-DL-003 | The component shall provide a command `BA_SetDrainLimit(maxOutstanding: U32)` that, for 1 ≤ maxOutstanding ≤ queue depth, sets the drain limit to maxOutstanding and responds OK. | Unit test; integration test |
| REQ-BA-DL-004 | Upon `BA_SetDrainLimit(maxOutstanding)` with maxOutstanding = 0 or maxOutstanding > queue depth, the component shall leave the drain limit unchanged, emit the WARNING_HI event `BA_InvalidDrainLimit(maxOutstanding, queue depth)`, and respond VALIDATION_ERROR. | Unit test; integration test |
| REQ-BA-DL-005 | The component shall produce the REQ-BA-DL-003 / 004 outcomes for `BA_SetDrainLimit` in ACCUMULATE mode, in DRAIN mode, and while a `BA_DrainBuffers` command is in progress. | Unit test |
| REQ-BA-DL-006 | While draining, after each buffer arrival on the fill input, each buffer arrival on the return input, each `BA_SetDrainLimit` command, and each command that starts draining, the component shall send queued buffers on the drain output, oldest first, until the queue is empty, or the outstanding count is greater than or equal to the drain limit, or (during `BA_DrainBuffers`) the partial-drain target has been reached. | Unit test |
| REQ-BA-DL-007 | The component shall not send a buffer on the drain output while the outstanding count is greater than or equal to the drain limit. | Unit test |
| REQ-BA-DL-008 | Upon receipt of a buffer on the return input, the component shall forward that buffer on the return output and, if the outstanding count is greater than 0, decrement the outstanding count by 1. | Unit test |
| REQ-BA-DL-009 | Upon receipt of a buffer on the return input while the outstanding count is 0, the component shall forward the buffer on the return output, keep the outstanding count at 0, and emit the WARNING_HI event `BA_UnexpectedReturn`, emitted only when the previous return-input arrival did not also find the count at 0. | Unit test |
| REQ-BA-DL-010 | When the mode is set to ACCUMULATE while buffers are outstanding, the component shall send no further queued buffers on the drain output and shall continue to apply REQ-BA-DL-008 to each returned buffer. | Unit test |
| REQ-BA-DL-011 | The component shall report the outstanding count on telemetry channel `BA_NumOutstandingBuffers` after initialization and each time the count changes. | Unit test |
| REQ-BA-DL-012 | The component shall report the drain limit on telemetry channel `BA_DrainLimit` after initialization and each time the limit changes. | Unit test; integration test |
| REQ-BA-DL-013 | With the drain limit at 1, for every command and port stimulus accepted by the pre-change component, the component shall produce the same sequence of drain-output sends, return-output sends, command responses, and pre-existing events and telemetry as the pre-change component. | Unit test (pre-change scenarios unchanged; private-state accesses retargeted) |
| REQ-BA-DL-014 | The DataProducts subtopology shall initialize `dpBufferAccumulator` with a deployment-configurable initial drain limit whose default value is 1. | Integration test |
| REQ-BA-DL-015 | A `BA_DrainBuffers` command shall complete — `BA_PartialDrainDone(numDrained)` emitted and command response OK — when the partial-drain target has been reached on the drain output, regardless of the outstanding count; a `BA_DrainBuffers` command received after that instant shall not be rejected with `BA_StillDraining`. | Unit test |
| REQ-BA-DL-016 | The outstanding count after initialization shall be 0. | Unit test |
| REQ-BA-DL-017 | The component shall assert if the queue-initialization call supplies an initial drain limit of 0 or greater than the queue depth. | Unit test |
| REQ-BA-DL-018 | Upon accepting `BA_SetDrainLimit(maxOutstanding)` the component shall emit the ACTIVITY_HI event `BA_DrainLimitSet(maxOutstanding)`. | Unit test; integration test |
| REQ-BA-DL-019 | The DataProducts subtopology configuration shall derive `dpWriter`'s message-queue depth from the `dpBufferAccumulator` queue depth such that it is at least the queue depth plus the messages `dpWriter`'s other queued inputs can have pending concurrently (named margin constant with rationale). | Inspection (`DataProductsConfig.fpp`) |

Terms: *queue depth* — `maxNumBuffers` supplied to `allocateQueue`; *outstanding buffer* — sent on `bufferSendOutDrain` and not yet received on `bufferSendInReturn`; *partial-drain target* — for `BA_DrainBuffers`, `numToDrain` when `blockMode = BLOCK`, otherwise the smaller of `numToDrain` and the number of queued buffers at command acceptance; *draining* — DRAIN mode, or a `BA_DrainBuffers` command whose target has not been reached.

## 2. Design

### 2.1 Component kind and ports

The component is `active`: buffer arrivals, buffer returns and commands are asynchronous messages handled on the component's own thread, so the FIFO, the outstanding count and the drain limit are touched by one thread only and need no lock.

| Port | Kind | Type | Purpose |
|---|---|---|---|
| `bufferSendInFill` | async input | `Fw.BufferSend` | Buffers to accumulate; enqueued on the FIFO (returned on `bufferSendOutReturn` if the FIFO is full). |
| `bufferSendInReturn` | async input | `Fw.BufferSend` | Drained buffers coming back from the consumer; forwarded on `bufferSendOutReturn`, outstanding count decremented. |
| `bufferSendOutDrain` | output | `Fw.BufferSend` | Drained buffers to the consumer. |
| `bufferSendOutReturn` | output | `Fw.BufferSend` | Buffers handed back to their owner. |
| `pingIn` / `pingOut` | async input (drop) / output | `Svc.Ping` | Health ping. |
| `cmdIn`, `cmdRegOut`, `cmdResponseOut`, `eventOut`, `eventOutText`, `tlmOut`, `timeCaller` | special | — | Command, event, telemetry and time infrastructure. |

Asynchronous because the ports are shared by producer and consumer threads; the queue depth is the deployment's choice (`queue size` on the instance).

### 2.2 State

| State | Type | Initial value | Set by |
|---|---|---|---|
| mode | `OpState` | `allocateQueue` `initialMode` (default ACCUMULATE) | `BA_SetMode` |
| FIFO of queued buffers | `ArrayFIFOBuffer` of `Fw::Buffer`, capacity `maxNumBuffers` | empty | fill input / drain sends |
| drain limit | `U32`, 1 .. `maxNumBuffers` (same type as `BA_SetDrainLimit.maxOutstanding` and `BA_DrainLimit`) | `allocateQueue` `initialDrainLimit` (default 1; 0 or > `maxNumBuffers` asserts, REQ-BA-DL-017) | `BA_SetDrainLimit` |
| outstanding count | `U32` (≤ drain limit at the moment of each send) | 0 (REQ-BA-DL-016) | drain sends (+1), return input (−1, floor 0) |
| partial drain target / sent so far | `FwSizeType` | 0 / 0 | `BA_DrainBuffers` |
| pending `BA_DrainBuffers` opcode / cmdSeq | `FwOpcodeType` / `U32` | — | `BA_DrainBuffers` (answered at completion, REQ-BA-DL-015, or on cancel by `BA_SetMode`) |
| unexpected-return flag | `bool` | `false` | return input: set when the arrival finds the count at 0, cleared when it finds the count > 0 (throttles `BA_UnexpectedReturn`) |

### 2.3 Behavior

**Drain gate (REQ-BA-DL-006/007).** A single routine *drain* runs after every fill arrival, return arrival, `BA_SetDrainLimit`, `BA_SetMode(DRAIN)` and accepted `BA_DrainBuffers`: while *draining* and `outstanding < drain limit` and (no partial drain, or sent-so-far < target) and the FIFO is non-empty, dequeue the oldest buffer, send it on `bufferSendOutDrain`, increment the outstanding count and (during a partial drain) sent-so-far. The loop is bounded by the FIFO depth. Nothing is sent when `outstanding >= drain limit`, so lowering the limit below the in-flight count simply pauses sending until enough buffers return.

The drain limit is capped at `maxNumBuffers` (REQ-BA-DL-004/017; human decision) so the topology has a fixed number to size the consumer against. Note the outstanding count is bounded by the drain limit, not by the FIFO: sent buffers leave the FIFO and new arrivals take their slots, so without the cap depth 10 / limit 20 reaches 20 in flight. The outstanding count — not the FIFO contents — bounds how many `bufferSendOutDrain` invocations the consumer may hold unprocessed (outstanding accumulates across bursts: send N, refill, send N more), and an FPP `async input` overflows with an assert by default. A topology must therefore give the consumer room for `maxNumBuffers` buffers on top of its other concurrent queued inputs. DataProducts does this by construction (REQ-BA-DL-019): `DataProductsConfig::QueueSizes::dpWriter = BufferAccumulator::maxNumBuffers + dpWriterQueueMargin` (margin = one `schedInInternal` message, which `dpWriter`'s sync `schedIn` handler posts to its own queue through an internal port, plus queued `CLEAR_EVENT_THROTTLE` commands), so a project overriding `maxNumBuffers` keeps the invariant. Other topologies own it directly (command annotation).

**Partial-drain completion (REQ-BA-DL-015).** When sent-so-far reaches the target, `BA_PartialDrainDone(numDrained)` is emitted and the `BA_DrainBuffers` response OK is sent immediately — the buffers need not have returned. This is the pre-change completion point. `BA_DrainStalled(sent, target)` is emitted every time *drain* is entered from `BA_DrainBuffers(BLOCK)` acceptance or from a return arrival with the partial-drain target unreached and the FIFO empty on entry — exactly the two pre-change emission points, unthrottled (REQ-BA-DL-013). It is not emitted on fill arrivals, on `BA_SetDrainLimit`, or when the loop stops at the drain limit. The limit check precedes the stall check: `BA_DrainStalled` is emitted only when the pass *would* send (`outstanding < drain limit`, partial-drain target unreached) but finds the FIFO empty; with `outstanding >= drain limit` the pass exits silently and the stall, if any, is detected on a later return (pre-change order). With N > 1 in flight, each of the N returns that finds the FIFO empty emits one warning; the burst is bounded by the drain limit.

**Return path (REQ-BA-DL-008/009).** Every buffer on `bufferSendInReturn` is forwarded on `bufferSendOutReturn`. If the outstanding count is > 0 it is decremented; otherwise it stays 0 and `BA_UnexpectedReturn` is emitted if the unexpected-return flag is clear (the flag — not a counter — implements the throttle; it is set here and cleared by a return that finds the count > 0). Worst case (every buffer returned twice): one `BA_UnexpectedReturn` per drained buffer. Then *drain* runs.

**Lost returns.** No timeout is applied to outstanding buffers. If buffers are never returned, the count stays high and sending pauses once it reaches the limit; this is visible on `BA_NumOutstandingBuffers`. There is no command to reset the count; the operator can restore flow with a higher `BA_SetDrainLimit` (the lost buffers remain counted) or by restarting the component. `BA_DrainStalled` does not cover this case (requirements RS-2 waiver).

**Mode change (REQ-BA-DL-010).** `BA_SetMode` (either value) first cancels an in-progress partial drain and responds OK to the original `BA_DrainBuffers` (pre-change behavior, REQ-BA-DL-013). `BA_SetMode(ACCUMULATE)` then stops further sends; outstanding buffers still return and decrement the count. `BA_SetMode(DRAIN)` runs *drain*.

**Drain limit command (REQ-BA-DL-003/004/005/018).** `BA_SetDrainLimit(maxOutstanding)`: 0 or > `maxNumBuffers` → `BA_InvalidDrainLimit(maxOutstanding, maxNumBuffers)`, VALIDATION_ERROR, limit unchanged; otherwise limit := maxOutstanding, `BA_DrainLimitSet(maxOutstanding)` (REQ-BA-DL-018), `BA_DrainLimit` telemetry, OK, then *drain*. Accepted in any mode and during a partial drain.

**Telemetry (REQ-BA-DL-011/012).** The initial `BA_DrainLimit` and `BA_NumOutstandingBuffers` values are written from `preamble()` (the `ActiveComponentBase` hook that runs on the component thread before the first dispatch, i.e. after `connectComponents()`), not from `allocateQueue`, which only stores them — `tlmWrite_*` on a not-yet-connected port is silently dropped, and the relative order of `configComponents()` and `connectComponents()` is chosen by each deployment's hand-written `setupTopology` (Ref connects first, but the component must not depend on it). Thereafter `BA_DrainLimit` is written when `BA_SetDrainLimit` changes it, and `BA_NumOutstandingBuffers` and `BA_NumQueuedBuffers` are written once at the end of each handler that changes them (after the *drain* loop), not per loop iteration.

**Backward compatibility (REQ-BA-DL-013).** With drain limit 1 the gate admits one buffer in flight, which is exactly the pre-change `m_waitForBuffer` behavior. The pre-change unit-test *scenarios* (stimuli and expected outputs) pass unchanged; only the tests' direct accesses to private state that the design removes (`m_send`, `m_waitForBuffer`) are retargeted to the new state (outstanding count).

### 2.4 Initialization

```cpp
void allocateQueue(FwEnumStoreType identifier,
                   Fw::MemAllocator& allocator,
                   FwSizeType maxNumBuffers,
                   BufferAccumulator_OpState initialMode = BufferAccumulator_OpState::ACCUMULATE,
                   U32 initialDrainLimit = 1);
```

`initialDrainLimit` must be in 1 .. `maxNumBuffers` (`FW_ASSERT`, REQ-BA-DL-017); the consumer must be sized for `maxNumBuffers` buffers in flight (see §2.3; a topology property, not checked by the component). `allocateQueue` stores the limit; its telemetry is first written from `preamble()` (§2.3). The DataProducts subtopology passes `DataProductsConfig::BufferAccumulator::initialDrainLimit` (default 1; REQ-BA-DL-014).

## 3. Dictionary



## Command List

|Mnemonic|ID|Description|Arg Name|Arg Type|Comment
|---|---|---|---|---|---|
|BA_SetMode|0 (0x0)|Set the mode| | |   
| | | |mode|OpState||                    
|BA_DrainBuffers|1 (0x1)|Drain the commanded number of buffers| | |   
| | | |numToDrain|U32||                    
| | | |blockMode|BlockMode||                    
|BA_SetDrainLimit|2 (0x2)|Set the drain limit: the maximum number of drained buffers that may be outstanding (sent on bufferSendOutDrain and not yet returned on bufferSendInReturn) at once. Applies to DRAIN mode and to BA_DrainBuffers. Accepted in any mode; 0 and values above the queue depth (maxNumBuffers given to allocateQueue) are rejected. The consumer connected to bufferSendOutDrain must be able to queue maxNumBuffers buffers in addition to its other concurrent queued inputs.| | |   
| | | |maxOutstanding|U32|The new drain limit; 1 .. maxNumBuffers|                    

## Telemetry Channel List

|Channel Name|ID|Type|Description|
|---|---|---|---|
|BA_NumQueuedBuffers|0 (0x0)|U32|The number of buffers queued|
|BA_NumOutstandingBuffers|1 (0x1)|U32|The number of drained buffers sent on bufferSendOutDrain and not yet returned on bufferSendInReturn|
|BA_DrainLimit|2 (0x2)|U32|The drain limit: the maximum number of drained buffers that may be outstanding at once|

## Event List

|Event Name|ID|Description|Arg Name|Arg Type|Arg Size|Description
|---|---|---|---|---|---|---|
|BA_BufferAccepted|0 (0x0)|The Buffer Accumulator instance accepted and enqueued a buffer. To avoid uncontrolled sending of events, this event occurs only when the previous buffer received caused a QueueFull error.| | | | |
|BA_QueueFull|1 (0x1)|The Buffer Accumulator instance received a buffer when its queue was full. To avoid uncontrolled sending of events, this event occurs only when the previous buffer received did not cause a QueueFull error.| | | | |
|BA_StillDraining|2 (0x2)|Got DrainBuffers command while executing DrainBuffers command| | | | |
| | | |numDrained|U32|||    
| | | |numToDrain|U32|||    
|BA_AlreadyDraining|3 (0x3)|Got DrainBuffers command while in DRAIN mode| | | | |
|BA_DrainStalled|4 (0x4)|Ran out of buffers while executing DrainBuffers command| | | | |
| | | |numDrained|U32|||    
| | | |numToDrain|U32|||    
|BA_PartialDrainDone|5 (0x5)|Finished DrainBuffers command| | | | |
| | | |numDrained|U32|||    
|BA_NonBlockDrain|6 (0x6)|Not enough buffers to complete requested drain, and NOBLOCK was set; will only drain what we have| | | | |
| | | |numWillDrain|U32|||    
| | | |numReqDrain|U32|||    
|BA_DrainLimitSet|7 (0x7)|BA_SetDrainLimit accepted; the drain limit is now maxOutstanding| | | | |
| | | |maxOutstanding|U32||The new drain limit|    
|BA_InvalidDrainLimit|8 (0x8)|BA_SetDrainLimit rejected: the requested drain limit is 0 or exceeds the queue depth; the drain limit is unchanged| | | | |
| | | |maxOutstanding|U32||The rejected value|    
| | | |maxAllowed|U32||The largest accepted value (the queue depth, maxNumBuffers)|    
|BA_UnexpectedReturn|9 (0x9)|A buffer arrived on bufferSendInReturn while no drained buffers were outstanding; it was forwarded on bufferSendOutReturn. To avoid uncontrolled sending of events, this event occurs only when the previous returned buffer did not also arrive with no buffers outstanding.| | | | |
