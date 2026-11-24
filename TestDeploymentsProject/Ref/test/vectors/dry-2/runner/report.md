# Runner report — run `dry-2`

Record `3cad12720afac16774ef25ed51f502de7f665993ba1e25eadcfe89d604349392` · checker `fth.junit` · 5 PASS / 1 FAIL / 0 UNVERIFIED

| test | title | verdict | failed checks |
|---|---|---|---|
| L0-1 | Image identity | PASS |  |
| L0-2 | Packets flow on change | PASS |  |
| L2-1 | Channel value reaches its first packet | FAIL | `cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them, a sample within 2 ticks after each dispatch` |
| L2-2 | Channel value reaches its second packet | PASS |  |
| L6-1 | Unknown packet is refused | PASS |  |
| L7-1 | Final state | PASS |  |

pytest exit code: 1
