# TPK-POOL — Svc.TlmPacketizer membership pool (issue 6431)

## 0 Scope
- UUT: `Svc.TlmPacketizer`; SHA in the manifest.
- Venue-neutral by ruling (ledger 14): tests use only `tlmSend`, `cmdDisp` and `version` commands, events and channels. Run in both venues: fprime-zephyr-reference on bench hardware and Ref on a Linux host; one record per venue.
- Test deployment delta, uncommitted to the UUT, same packet-set change in every venue: `cmdDisp.CommandsDispatched` is removed from the packet that holds it today (`Health` id 1 in fprime-zephyr-reference, `CDH` id 1 in Ref) and listed in exactly two new packets, `TpkA` id 60 and `TpkB` id 61, both in group 1. The channel has two memberships and no other.
- Delta per venue, fprime-zephyr-reference @ 6444eb6: `tlmSend` is already `Svc.TlmPacketizer` (with `IGNORE_OMIT_LIST`); `MAX_PACKETIZER_PACKETS` 5 → 7; `MAX_PACKETIZER_CHANNELS` 40 unchanged; `MAX_PACKETIZER_MEMBERSHIPS` new = 31 (30 entries today − 1 + 2) and 30 channels listed, counted from the packet set; the build's sizing check confirms them at the first venue build.
- Delta per venue, Ref: `tlmSend` switched from `Svc.TlmChan` to `Svc.TlmPacketizer` with a `setPacketList` call given `IGNORE_OMIT_LIST` (build option `FPRIME_TLM_PACKETIZER_OMIT_LIST=OFF`), as the other venue does; `tlmSend.GroupConfigs` and `tlmSend.SectionEnabled` listed in packet `CDH`; 37 packets, 178 memberships (175 − 1 + 2 + 2) and 177 channels, under the defaults 50 and 200; counts confirmed from the build's sizing check. With the omit list counted the same packet set needs 202 channel slots (177 listed + 25 omitted).
- Out of scope: the build-time sizing check (pytest in `cmake/test`, not a system test); rate logic and override persistence (unchanged).

## 1 Entry gates
- L0 desk check: the longest window (L2-1) holds 2 markers + 4 commands = 6 commands, about 14 event rows and 2 channel samples; no downlink budget is approached in either venue. Bench confirmation pending at dry run.
- Group 1 of section REALTIME sends on change in the image (default `ON_CHANGE_MIN`); confirmed at dry run by L0-2, else the plan returns to design.
- Telemetry tick: `cmdDisp` writes `CommandsDispatched` from its rate-group handler, 1 s in both venues (helper argument `tick_s`); confirmed against each topology at dry run.
- Stage-1 close: `fprime-util check` on `Svc/TlmPacketizer` passes with every unit-test row of the SDD implemented.
- Marker round-trip: `CMD_NO_OP_STRING("<n>")` returns `NoOpStringReceived` with text `Received a NO-OP string=<n>` byte-exact through the archive.
- Image built from the manifest SHAs; dictionary from that build.

## 2 Test levels
L0 provenance/feasibility · L2 functional · L6 negative controls · L7 final state. L3 (reset), L4 (endurance) and L5 (fault paths) have no test: the change has no persistent state, no timing requirement, and its only fault path is a boot assert that needs a different image.

## 3 Test entries

Window rule: rows from the **last** `cmdDisp.NoOpStringReceived(id=START)` row that precedes the first `cmdDisp.NoOpStringReceived(id=END)` row following it, through that END row plus the END `cmdDisp.CMD_NO_OP_STRING` command's own `cmdDisp` rows (at most two), inclusive; ordering key `t = epoch·1e6 + fsw`, archive index on ties. Channel rows are those with `t` between the first and last window event. `commands_sent` = commands from the matching START sending (its last sending if re-sent) through the END sending, inclusive.

Standard `complete` block (STD): both markers present; commands_sent == count(event cmdDisp.OpCodeDispatched); event cmdDisp.OpCodeError count == 0.
Standard absence block (ABS): event tlmSend.NoChan count == 0; event tlmSend.PacketNotFound count == 0.

ID:        L0-1
Title:     Image identity
Purpose:   Shows the running image is the one built from the manifest's commits.
Covers:    —
Setup:     Image running, ground system attached.
Procedure: MARK(10); VERSION(ALL); MARK(11).
Window:    Marker(id=10) … Marker(id=11)
Pass rule: complete: STD. all_of: event version.FrameworkVersion count >= 1.
Evidence:  Version events equal to the manifest's describe strings.
Planted faults:none

ID:        L0-2
Title:     Packets flow on change
Purpose:   Shows the image boots with the two-packet channel and sends it on change.
Covers:    —
Setup:     As L0-1.
Procedure: MARK(20); CMD_NO_OP(); wait 1 CommandsDispatched sample; MARK(21).
Window:    Marker(id=20) … Marker(id=21)
Pass rule: complete: STD. all_of: ABS; cmdDisp.CommandsDispatched has >= 1 sample in the window.
Evidence:  A CommandsDispatched sample inside the window; no NoChan event.
Planted faults:none

ID:        L2-1
Title:     Channel value reaches its first packet
Purpose:   Shows channel updates are stored in, and downlinked by, the first packet listing the channel.
Covers:    TPK-001, TPK-003
Setup:     L0-2 passed.
Procedure: OVERRIDE_PACKET_SEND(REALTIME, 61, SEND_NEVER); wait 2 ticks; MARK(30); CMD_NO_OP(); wait 1 CommandsDispatched sample; CMD_NO_OP(); wait 1 CommandsDispatched sample; MARK(31); wait 2 ticks; CLEAR_PACKET_OVERRIDE(REALTIME, 61).
Window:    Marker(id=30) … Marker(id=31)
Pass rule: complete: STD. all_of: ABS; cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them (the channel is listed only in packets 60 and 61; packet 61 is set SEND_NEVER two ticks before the start marker and released two ticks after the end marker, so every sample inside the window is downlinked by packet 60. The end marker is itself a dispatched command, so the held packet's pending sample carries a time after the window and its flush on release is excluded. A membership that is not written leaves packet 60 unchanged, it is not sent, and the check fails on sample count).
Evidence:  At least two rising CommandsDispatched samples, each step equal to the OpCodeDispatched rows archived between them.
Planted faults:first_packet_also_held

ID:        L2-2
Title:     Channel value reaches its second packet
Purpose:   Shows the same channel's updates are also stored in the second packet listing it.
Covers:    TPK-001, TPK-003
Setup:     L2-1 run.
Procedure: OVERRIDE_PACKET_SEND(REALTIME, 60, SEND_NEVER); wait 2 ticks; MARK(40); CMD_NO_OP(); wait 1 CommandsDispatched sample; CMD_NO_OP(); wait 1 CommandsDispatched sample; MARK(41); wait 2 ticks; CLEAR_PACKET_OVERRIDE(REALTIME, 60).
Window:    Marker(id=40) … Marker(id=41)
Pass rule: complete: STD. all_of: ABS; cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them (the channel is listed only in packets 60 and 61; packet 60 is set SEND_NEVER two ticks before the start marker and released two ticks after the end marker, so every sample inside the window is downlinked by packet 61. The end marker is itself a dispatched command, so the held packet's pending sample carries a time after the window and its flush on release is excluded. A membership that is not written leaves packet 61 unchanged, it is not sent, and the check fails on sample count).
Evidence:  At least two rising CommandsDispatched samples, each step equal to the OpCodeDispatched rows archived between them.
Planted faults:second_packet_also_held

ID:        L6-1
Title:     Unknown packet is refused
Purpose:   Shows a send request for an unknown packet is refused and reported.
Covers:    —
Setup:     L2-2 run.
Procedure: MARK(50); SEND_PKT(999, REALTIME); MARK(51).
Window:    Marker(id=50) … Marker(id=51)
Pass rule: complete: both markers present; commands_sent == count(event cmdDisp.OpCodeDispatched). all_of: event cmdDisp.OpCodeError count == 1 (SEND_PKT for an unknown id answers VALIDATION_ERROR, so this test does not use STD (which requires zero OpCodeError) and expects exactly one); event tlmSend.PacketNotFound(packetId=999) count == 1; event tlmSend.PacketSent count == 0; event tlmSend.NoChan count == 0.
Evidence:  PacketNotFound(999) once, one OpCodeError, no PacketSent.
Planted faults:known_packet_requested

ID:        L7-1
Title:     Final state
Purpose:   Leaves both test packets enabled; the channel's value still follows the archive's dispatch count.
Covers:    —
Setup:     All earlier tests run.
Procedure: CLEAR_PACKET_OVERRIDE(REALTIME, 60); CLEAR_PACKET_OVERRIDE(REALTIME, 61); wait 2 ticks; MARK(60); CMD_NO_OP(); wait 1 CommandsDispatched sample; CMD_NO_OP(); wait 1 CommandsDispatched sample; MARK(61).
Window:    Marker(id=60) … Marker(id=61)
Pass rule: complete: STD. all_of: ABS; cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them.
Evidence:  Rising CommandsDispatched samples that match the OpCodeDispatched rows the ground archive holds (a path independent of the packetizer's storage).
Planted faults:final_packets_held

## 4 Traceability
| REQ | Tests | Note |
|---|---|---|
| TPK-001 | L2-1, L2-2 + UT | Unit Test, System test |
| TPK-003 | L2-1, L2-2 + UT | Unit Test, System test |
| TPK-013 | waived: UT only | The limit and its assert can only be reached with an image whose packet list overflows the pool, which is a different build; unit tests `membershipCapacityExactTest` and `membershipOverflowTest` verify it. No system test is traced to it. |

## 5 Metrics
- (none)

## 6 Fault menu
Generated from `planted_faults.py` planted fault declarations (`fth planted-faults-menu`): final_packets_held (uut-state; L7-1); first_packet_also_held (uut-state; L2-1); known_packet_requested (stimulus; L6-1); second_packet_also_held (uut-state; L2-2).

## 7 Not covered
- TPK-013 (limit and assert), and every other setPacketList assert: needs a different image; unit tests.
- Memory saved by the pool: not observable through commands or telemetry; shown by the SDD sizing table and the build's map file (inspection).
- The build-time sizing check: verified by pytest in `cmake/test` and a compile-failure fixture, not on a bench.
- TPK-002, 004..012: unchanged behavior, existing unit tests; no system test added by this issue.
- State after a drawn `final_packets_held` fault: the test packets stay held back until reboot; the bench is power-cycled or reflashed after every run for record.

## 8 Decision log
1. 2026-11-23 approver — venues: fprime-zephyr-reference on bench hardware and Ref on a Linux host (ledger 12).
2. 2026-11-23 approver — the tests run in both venues; the venue is a run artifact and stays out of the SDD (ledger 14).
3. 2026-11-23 designer — marker = `cmdDisp.CMD_NO_OP_STRING`; no tester component is added.
4. 2026-11-23 designer — test-deployment delta of the scope section (two test packets, the channel moved into them, sizing constants per venue); for the approver at gate 1.
5. 2026-11-23 designer — system coverage threshold provisionally 50 % lines; for gate 1.
6. 2026-11-24 designer, review round 1 — packets are sent by the rate logic, not SEND_PKT (SEND_PKT bypasses enable checks and logs PacketSent on acceptance); menu max_count 2 over four faulted tests; TPK-013 is UT only with no system test traced.
7. 2026-11-24 designer, review round 2 — the hold-back and its release sit outside the L2 windows, two ticks clear of the markers.
