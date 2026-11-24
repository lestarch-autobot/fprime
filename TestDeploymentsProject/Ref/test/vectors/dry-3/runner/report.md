# Runner report — run `dry-3`

Record `69bf303640f9fd46719437244736f5f75b8a954fd1ec0beaf004ce66c33f82b7` · checker `fth.junit` · 5 PASS / 1 FAIL / 0 UNVERIFIED

| test | title | verdict | failed checks |
|---|---|---|---|
| L0-1 | Image identity | PASS |  |
| L0-2 | Packets flow on change | PASS |  |
| L2-1 | Channel value reaches its first packet | PASS |  |
| L2-2 | Channel value reaches its second packet | FAIL | `cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them, a sample within 2 ticks after each dispatch` |
| L6-1 | Unknown packet is refused | PASS |  |
| L7-1 | Final state | PASS |  |

pytest exit code: 1
