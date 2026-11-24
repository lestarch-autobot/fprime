# Runner (live) vs replay (auditor)

| test | runner (live) | replay (auditor) | agree |
|---|---|---|---|
| L0-1 | PASS | PASS | yes |
| L0-2 | PASS | PASS | yes |
| L2-1 | FAIL cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them, a sample within 2 ticks after each dispatch | FAIL cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them, a sample within 2 ticks after each dispatch | yes |
| L2-2 | PASS | PASS | yes |
| L6-1 | PASS | PASS | yes |
| L7-1 | PASS | PASS | yes |
