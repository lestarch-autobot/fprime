# TPK-POOL — Svc.TlmPacketizer membership pool (issue 6431)

## 0 Scope
- UUT: `Svc.TlmPacketizer`; SHA in the manifest.
- Venue-neutral by ruling (ledger 14): tests use only `tlmSend`, `cmdDisp` and `version` commands, events and channels. Venues for the runs: fprime-zephyr-reference on bench hardware (prime), Ref on a Linux host (fallback); both recorded per run.
- Test deployment delta (same in every venue, uncommitted to the UUT): `tlmSend` is `Svc.TlmPacketizer`; packet set adds `TpkA` id 60 and `TpkB` id 61, each holding `cmdDisp.CommandsDispatched` (one channel, two memberships), and `TpkC` id 62 holding only `cmdDisp.CommandsDropped`; all three in group 1.
- Out of scope: the build-time sizing check (pytest in `cmake/test`, not a system test); rate logic and override persistence (unchanged).

## 1 Entry gates
- L0: desk-checked; bench confirmation pending.
- Stage-1 close: `fprime-util check` on `Svc/TlmPacketizer` passes with every unit-test row of the SDD implemented.
- Marker round-trip: `CMD_NO_OP_STRING("<n>")` returns `NoOpStringReceived` byte-exact through the archive.
- Image built from the manifest SHAs; dictionary from that build.

## 2 Test levels
L0 provenance/feasibility · L2 functional · L6 negative controls · L7 final state. L3 (reset), L4 (endurance) and L5 (fault paths) have no test: the change has no persistent state, no timing requirement, and its only fault path is a boot assert that needs a different image.

## 3 Test entries

Window rule: rows from the **last** `cmdDisp.NoOpStringReceived(id=START)` row that precedes the first `cmdDisp.NoOpStringReceived(id=END)` row following it, through that END row plus the END `cmdDisp.CMD_NO_OP_STRING` command's own `cmdDisp` rows (at most two), inclusive; ordering key `t = epoch·1e6 + fsw`, archive index on ties. Channel rows are those with `t` between the first and last window event. `commands_sent` = commands from the matching START sending (its last sending if re-sent) through the END sending, inclusive.

Standard `complete` block (STD): both markers present; commands_sent == count(event cmdDisp.OpCodeDispatched); event cmdDisp.OpCodeError count == 0.
Standard absence block (ABS): event tlmSend.NoChan count == 0; event tlmSend.PacketNotFound count == 0.
Packets (named channel sets for `<packet> samples >= n`): TpkA = cmdDisp.CommandsDispatched.

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
Title:     Packet list accepted at boot
Purpose:   Shows a packet list with a channel in two packets is accepted and packets flow.
Covers:    TPK-013
Setup:     As L0-1.
Procedure: MARK(20); bracket (2 TpkA samples); MARK(21).
Window:    Marker(id=20) … Marker(id=21)
Pass rule: named checks declared in the integration test (test_L0_2).
Evidence:  TpkA samples inside the window; no NoChan event.
Planted faults:none

ID:        L2-1
Title:     Channel value reaches its first packet
Purpose:   Shows a channel update is stored in, and downlinked by, the first packet that lists it.
Covers:    TPK-001, TPK-003
Setup:     L0-2 passed.
Procedure: MARK(30); OVERRIDE_PACKET_SEND(REALTIME, 61, DISABLED); SEND_PKT(60, REALTIME); CLEAR_PACKET_OVERRIDE(REALTIME, 61); MARK(31).
Window:    Marker(id=30) … Marker(id=31)
Pass rule: named checks declared in the integration test (test_L2_1).
Evidence:  PacketSent(60) once, then a CommandsDispatched sample whose value counts the window's commands.
Planted faults:send_other_packet

ID:        L2-2
Title:     Channel value reaches its second packet
Purpose:   Shows the same channel update is also stored in the second packet that lists it.
Covers:    TPK-001, TPK-003
Setup:     L2-1 run.
Procedure: MARK(40); OVERRIDE_PACKET_SEND(REALTIME, 60, DISABLED); SEND_PKT(61, REALTIME); CLEAR_PACKET_OVERRIDE(REALTIME, 60); MARK(41).
Window:    Marker(id=40) … Marker(id=41)
Pass rule: named checks declared in the integration test (test_L2_2).
Evidence:  PacketSent(61) once, then a CommandsDispatched sample equal to the dispatcher's own count.
Planted faults:second_packet_left_disabled

ID:        L6-1
Title:     Unknown packet is refused
Purpose:   Shows a send request for an unknown packet is reported and sends nothing.
Covers:    —
Setup:     L2-2 run.
Procedure: MARK(50); SEND_PKT(999, REALTIME); MARK(51).
Window:    Marker(id=50) … Marker(id=51)
Pass rule: named checks declared in the integration test (test_L6_1).
Evidence:  PacketNotFound(999) once; no PacketSent.
Planted faults:none

ID:        L7-1
Title:     Final state
Purpose:   Leaves both test packets enabled and shows each still carries the channel.
Covers:    —
Setup:     All earlier tests run.
Procedure: MARK(60); ENABLE_SECTION(REALTIME, ENABLED); CLEAR_PACKET_OVERRIDE(REALTIME, 60); CLEAR_PACKET_OVERRIDE(REALTIME, 61); SEND_PKT(60, REALTIME); SEND_PKT(61, REALTIME); MARK(61).
Window:    Marker(id=60) … Marker(id=61)
Pass rule: named checks declared in the integration test (test_L7_1).
Evidence:  PacketSent(60) and PacketSent(61); the last CommandsDispatched sample equals the count of OpCodeDispatched events the ground archive holds since boot (independent of the packetizer).
Planted faults:none

## 4 Traceability
| REQ | Tests | Note |
|---|---|---|
| TPK-001 | L2-1, L2-2 + UT | Unit Test, System test |
| TPK-003 | L2-1, L2-2 + UT | Unit Test, System test |
| TPK-013 | UT only | Accept side seen at L0-2 (the image boots and sends with a two-membership channel); the assert side needs an image that overflows the pool, which is a different build, so it is a unit death test. No second planted-fault-disjoint system test is possible. |

## 5 Metrics
- (none)

## 6 Fault menu
Generated from `planted_faults.py` planted fault declarations (`fth planted-faults-menu`): second_packet_left_disabled (uut-state; L2-2); send_other_packet (stimulus; L2-1).

## 7 Not covered
- TPK-013 assert side, and every other setPacketList assert: needs a different image; unit death tests.
- Memory saved by the pool: not observable through commands or telemetry; shown by the SDD sizing table and the build's map file (inspection).
- The build-time sizing check: verified by pytest in `cmake/test` and a compile-failure fixture, not on a bench.
- TPK-002, 004..012: unchanged behavior, existing unit tests; no system test added by this issue.

## 8 Decision log
1. 2026-11-23 approver — venue: fprime-zephyr-reference on bench hardware, fallback Ref on a Linux host (ledger 12).
2. 2026-11-23 approver — tests run in both venues; the venue is a run artifact and stays out of the SDD (ledger 14).
3. 2026-11-23 designer — marker = `cmdDisp.CMD_NO_OP_STRING`; no tester component is added.
4. 2026-11-23 designer — test packets TpkA/TpkB/TpkC are added to the test deployment only; for the approver at gate 1.
5. 2026-11-23 designer — system coverage threshold 50 % lines (see coverage.rationale); for the design review and gate 1.
