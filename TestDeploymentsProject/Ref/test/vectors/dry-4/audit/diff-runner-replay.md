# Runner (live) vs replay (auditor)

| test | runner (live) | replay (auditor) | agree |
|---|---|---|---|
| L0-1 | PASS | PASS | yes |
| L0-2 | PASS | PASS | yes |
| L2-1 | PASS | PASS | yes |
| L2-2 | PASS | PASS | yes |
| L6-1 | FAIL event cmdDisp.OpCodeError count == 1, event tlmSend.PacketNotFound(packetId=999) count == 1, event tlmSend.PacketSent count == 0 | FAIL event cmdDisp.OpCodeError count == 1, event tlmSend.PacketNotFound(packetId=999) count == 1, event tlmSend.PacketSent count == 0 | yes |
| L7-1 | PASS | PASS | yes |
