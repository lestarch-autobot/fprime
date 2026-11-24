# Unit-test and build-check list (TlmPacketizer membership pool)

Kept out of the SDD by ruling at the Approve SDD gate (2026-11-24). Reviewed with the test design.

| Test | Description | Requirements |
|---|---|---|
| UT `membershipCapacityExactTest` | A packet list of exactly `MAX_PACKETIZER_MEMBERSHIPS` entries is accepted. | TPK-013 |
| UT `membershipOverflowTest` | One entry more asserts, reporting the needed total first. | TPK-013 |
| UT `omittedChannelsUseNoPoolEntryTest` | Omitted channels take a channel slot and no pool entry. | TPK-013 |
| UT `channelInTwoPacketsTest` | A channel listed in two packets is written to both. | TPK-001, TPK-003 |
| UT `channelTwiceInOnePacketTest` | A channel listed twice in one packet is written at both offsets. | TPK-003 |
| UT `tlmGetFirstMembershipTest` | `TlmGet` returns the value from the channel's first packet. | TPK-003 |
| UT `tlmGetOmittedChannelTest` | `TlmGet` on an omitted channel returns invalid and no `NoChan`. | TPK-003 |
| UT `ignoreOmitListTest` | With `IGNORE_OMIT_LIST`, omitted channels take no channel slot. | TPK-001 |
| UT `portsBeforeSetPacketListAssertTest` | `TlmRecv`, `TlmGet` and `Run` assert before `setPacketList`. | TPK-013 |
| UT existing suite | Tests on `devel` pass; only the tester's internal-table accessors change. | TPK-001..TPK-012 |
| Build check, over memberships | A dictionary over `MAX_PACKETIZER_MEMBERSHIPS` fails the deployment build. | TPK-013 |
| Build check, over channels | With the omit-list variable `ON`, omitted channels count and fail the build. | TPK-013 |
| Build check, omit list off | With the variable `OFF`, the same dictionary builds. | TPK-013 |
