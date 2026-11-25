# TPK-POOL — Svc.TlmPacketizer membership pool (issue 6431)

## 0 Scope
- UUT: `Svc.TlmPacketizer`; SHA in the manifest.
- Venue-neutral by ruling (ledger 14): tests use only `tlmSend`, `cmdDisp` and `version` commands, events and channels. Run in both venues: fprime-zephyr-reference on bench hardware and Ref on a Linux host; one record per venue.
- Test deployment delta, uncommitted to the UUT, same packet-set change in every venue: `cmdDisp.CommandsDispatched` is removed from the packet that holds it today (`Health` id 1 in fprime-zephyr-reference, `CDH` id 1 in Ref) and listed in exactly two new packets, `TpkA` id 60 and `TpkB` id 61, both in group 1. The channel has two memberships and no other.
- Delta per venue, fprime-zephyr-reference @ 6444eb6: `tlmSend` is already `Svc.TlmPacketizer` (with `IGNORE_OMIT_LIST`); `MAX_PACKETIZER_PACKETS` 5 → 7; `MAX_PACKETIZER_CHANNELS` 40 unchanged; `MAX_PACKETIZER_MEMBERSHIPS` new = 31 (30 entries today − 1 + 2) and 30 channels listed, counted from the packet set; the build's sizing check confirms them at the first venue build.
- Delta per venue, fprime-zephyr-reference @ 6444eb6, command dispatcher tick: the topology there leaves `CdhCore.Subtopology.cmdDispRun` unconnected, so `cmdDisp` never writes its telemetry; the test deployment adds the one connection `rateGroup1Hz.RateGroupMemberOut[6] -> CdhCore.Subtopology.cmdDispRun`, as Ref has on its 1 Hz rate group, and nothing else in the topology changes.
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
Pass rule: named checks declared in the integration test (test_L0_1).
Evidence:  Version events equal to the manifest's describe strings.
Planted faults:none

ID:        L0-2
Title:     Packets flow on change
Purpose:   Shows the image boots with the two-packet channel and sends it on change.
Covers:    —
Setup:     As L0-1.
Procedure: MARK(20); CMD_NO_OP(); wait 3 ticks; MARK(21).
Window:    Marker(id=20) … Marker(id=21)
Pass rule: named checks declared in the integration test (test_L0_2).
Evidence:  A CommandsDispatched sample inside the window; no NoChan event.
Planted faults:none

ID:        L2-1
Title:     Channel value reaches its first packet
Purpose:   Shows channel updates are stored in, and downlinked by, the first packet listing the channel.
Covers:    TPK-001, TPK-003
Setup:     L0-2 passed.
Procedure: MARK(29); OVERRIDE_PACKET_SEND(REALTIME, 61, SEND_NEVER); wait 2 ticks; MARK(30); CMD_NO_OP(); wait 3 ticks; CMD_NO_OP(); wait 3 ticks; MARK(31); wait 2 ticks; CLEAR_PACKET_OVERRIDE(REALTIME, 61); MARK(32).
Window:    S = Marker(id=29) … Marker(id=30); W = Marker(id=30) … Marker(id=31); T = Marker(id=31) … Marker(id=32)
Pass rule: named checks declared in the integration test (test_L2_1).
Evidence:  At least two rising CommandsDispatched samples, each step equal to the OpCodeDispatched rows archived between them.
Planted faults:first_packet_also_held

ID:        L2-2
Title:     Channel value reaches its second packet
Purpose:   Shows the same channel's updates are also stored in the second packet listing it.
Covers:    TPK-001, TPK-003
Setup:     L2-1 run.
Procedure: MARK(39); OVERRIDE_PACKET_SEND(REALTIME, 60, SEND_NEVER); wait 2 ticks; MARK(40); CMD_NO_OP(); wait 3 ticks; CMD_NO_OP(); wait 3 ticks; MARK(41); wait 2 ticks; CLEAR_PACKET_OVERRIDE(REALTIME, 60); MARK(42).
Window:    S = Marker(id=39) … Marker(id=40); W = Marker(id=40) … Marker(id=41); T = Marker(id=41) … Marker(id=42)
Pass rule: named checks declared in the integration test (test_L2_2).
Evidence:  At least two rising CommandsDispatched samples, each step equal to the OpCodeDispatched rows archived between them.
Planted faults:second_packet_also_held

ID:        L6-1
Title:     Unknown packet is refused
Purpose:   Shows a send request for an unknown packet is refused and reported.
Covers:    —
Setup:     L2-2 run.
Procedure: MARK(50); SEND_PKT(999, REALTIME); MARK(51).
Window:    Marker(id=50) … Marker(id=51)
Pass rule: named checks declared in the integration test (test_L6_1).
Evidence:  PacketNotFound(999) once, one OpCodeError, no PacketSent.
Planted faults:known_packet_requested

ID:        L7-1
Title:     Final state
Purpose:   Leaves both test packets enabled; the channel's value still follows the archive's dispatch count.
Covers:    —
Setup:     All earlier tests run.
Procedure: MARK(59); CLEAR_PACKET_OVERRIDE(REALTIME, 60); CLEAR_PACKET_OVERRIDE(REALTIME, 61); wait 2 ticks; MARK(60); CMD_NO_OP(); wait 3 ticks; CMD_NO_OP(); wait 3 ticks; MARK(61).
Window:    S = Marker(id=59) … Marker(id=60); W = Marker(id=60) … Marker(id=61)
Pass rule: named checks declared in the integration test (test_L7_1).
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
- A packet sent again although none of its channels changed: not judged (the helper drops the copy, decision 12).
- A reboot of the unit during a run: no test window; with `reboot_min_backstep_s` 60 s the export does not mark one whose time steps back by less than 60 s (decision 13).

## 8 Decision log
1. 2026-11-23 approver — venues: fprime-zephyr-reference on bench hardware and Ref on a Linux host (ledger 12).
2. 2026-11-23 approver — the tests run in both venues; the venue is a run artifact and stays out of the SDD (ledger 14).
3. 2026-11-23 designer — marker = `cmdDisp.CMD_NO_OP_STRING`; no tester component is added.
4. 2026-11-23 designer — test-deployment delta of the scope section (two test packets, the channel moved into them, sizing constants per venue); for the approver at gate 1.
5. 2026-11-23 designer — system coverage threshold provisionally 50 % lines; for gate 1.
6. 2026-11-24 designer, review round 1 — packets are sent by the rate logic, not SEND_PKT (SEND_PKT bypasses enable checks and logs PacketSent on acceptance); menu max_count 2 over four faulted tests; TPK-013 is UT only with no system test traced.
7. 2026-11-24 designer, review round 2 — the hold-back and its release sit outside the L2 windows, two ticks clear of the markers.
8. 2026-11-24 harness builder, venue A dry runs — the tooling's record integrity accepts no command outside a declared window, so the hold-back and release commands of L2-1 and L2-2 and the two clears of L7-1 are bracketed by setup (`S`) and teardown (`T`) windows that share an edge with the measurement window `W`; they are judged for completeness and for no `PacketNotFound` only. `W`, its markers, checks and planted faults are unchanged.
9. 2026-11-24 approver ruling, venue A coverage campaign — a plan that keeps `complete`/`all_of` is judged by the tooling's engine and its audit builds no runner-versus-replay table, so the pass rules move, unchanged, to the named checks of the integration tests (`require` for the former `complete` entries, `check` for the former `all_of` entries; `test_L*.py` beside this plan). `amendment-4-equivalence.md` lists every old entry against its new call: 26 identical, 4 helper checks reworded in form only, none added, none dropped. Windows, markers, groups `STD` and `ABS`, rationale, planted faults and their `fails` sets, thresholds, coverage and not-covered entries are unchanged.
10. 2026-11-24 approver ruling, venue B dry run — the pacing step `wait 1 CommandsDispatched sample` of L0-2, L2-1, L2-2 and L7-1 becomes `wait 2 ticks`, the fixed wait the plan already uses before and after its windows. A wait for one more sample starts counting after the command has completed; when the 1 Hz sample of that command is downlinked before the completion is seen, no further sample follows (the packet is sent on change only) and the wait expires although the unit behaved. Pacing only: windows, markers, commands, checks, planted faults and their `fails` sets are unchanged. The checks are judged over the closed window, so a sample later than two ticks is outside the window and the check fails.
11. 2026-11-24 test-design review of entry 10, `check-gap:last-update-not-judged` — with the sample wait gone, nothing judged a sample missing after a command (the start marker's own sample kept the count at two). The helper check of L2-1, L2-2 and L7-1 now also requires, for every dispatch at least two ticks before the last one in the window (the end marker's), a sample of its own within two ticks after it, judged on the sample's time; its name gains `, a sample within 2 ticks after each dispatch` and the three `fails` sets carry the new name. The seven pacing waits of entry 10 become `wait 3 ticks`, one tick more than the bound the check judges, so that the live run and its replay see the same samples. L0-2 keeps its check (at least one sample in the window) unchanged.
12. 2026-11-24 harness review rounds 1 and 2, `helper-duplicate-samples` — `TpkA` and `TpkB` both carry `cmdDisp.CommandsDispatched`, so with both enabled (L0-2, L7-1) one channel write is downlinked twice with the same time tag and value. The tool's export collapses the two copies only when their recorder lines are adjacent; on `Ref` the write (rate-group thread) can fall between the packetizer's visits of the two packets, the copies then leave one tick apart, both rows reach the window, and the helper judged a step of 0: a correct unit could FAIL. The helper `channel_tracks_dispatch_count` now drops a sample equal in time and value to the one before it, before any rule is applied. A dropped row is a second copy of one channel write: the time tag is the time of the write, so an equal time and value cannot come from a later write, and a repeat of the value with a later time is kept and judged as before. No check name, `fails` set, window, command or wait changes; `faults-menu.json` is unchanged. Not judged by this helper after the change: a packet sent again although its channel did not change (before, such a resend failed the check by accident; the plan has no check for it).
13. 2026-11-24 harness dry run at amendment 7, `L7-1 seen 0 samples` — `reboot_min_backstep_s` is 60 s (the tool's default is 5 s). The record export starts a new boot epoch for all later channel rows when a channel row is older than the newest one by more than this limit, and a window then holds no channel row. A held test packet released in the teardown of L2-1 or L2-2 carries the time of its last write, 4 to 6 ticks old on `Ref` depending on command latency; at 6 ticks a correct unit failed L7-1 (and could fail L2-2) with `0 samples`, in 2 of 6 dry runs. No test, check, window, wait, helper or fault changes; the teardown wait is kept; `faults-menu.json` is unchanged. Cost: an FSW-time backstep of 5 to 60 s (a reboot within the first minute after boot on a venue whose time starts at 0) no longer starts a new epoch; the plan has no reboot window, and a reboot inside a run still shows as lost commands and a second boot's events. Verdicts exported with the 5 s limit are not evidence; records are re-judged from their raw logs.
14. 2026-11-24 release, `fth deliver` — `image_version.harness_in_image` is false. This campaign has no tester component: `fprime-testing-harness` is the test tool only and contributes no source to either image, so its describe string cannot be in an image. The describe strings of the unit and of the project (the test deployment tree) stay required in the image. The manifest names the tool checkout as `harness` and the test deployment tree as `project`; the tests lie outside the tool checkout. No test, check, window, wait, helper or fault changes; `faults-menu.json` is unchanged.
