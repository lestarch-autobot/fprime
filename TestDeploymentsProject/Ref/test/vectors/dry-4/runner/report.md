# Runner report — run `dry-4`

Record `df13dc7d9f05edb4fee5a1547a753b8a335400e3355f271baefae394478bd986` · checker `fth.junit` · 5 PASS / 1 FAIL / 0 UNVERIFIED

| test | title | verdict | failed checks |
|---|---|---|---|
| L0-1 | Image identity | PASS |  |
| L0-2 | Packets flow on change | PASS |  |
| L2-1 | Channel value reaches its first packet | PASS |  |
| L2-2 | Channel value reaches its second packet | PASS |  |
| L6-1 | Unknown packet is refused | FAIL | `event cmdDisp.OpCodeError count == 1`<br>`event tlmSend.PacketNotFound(packetId=999) count == 1`<br>`event tlmSend.PacketSent count == 0` |
| L7-1 | Final state | PASS |  |

pytest exit code: 1
