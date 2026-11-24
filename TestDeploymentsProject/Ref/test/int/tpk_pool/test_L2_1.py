"""L2-1 — Channel value reaches its first packet (plan.json beside this file is the baseline; this is its stimulus).

The checks declared here are, verbatim, the plan's `complete` (require) and `all_of` (check) entries
of L2-1; the plan stays the owner of the pass rule. Waits are pacing only.
"""
import pytest


@pytest.mark.req("TPK-001", "TPK-003")
def test_L2_1(window, send, bench, check, require, fth_plan):
    send("tlmSend.OVERRIDE_PACKET_SEND", ["REALTIME", 61, "SEND_NEVER"])
    bench.wait(2.5)  # wait 2 ticks (1 s tick) with half a tick of margin
    with window(30, 31):
        require("STD")
        send("cmdDisp.CMD_NO_OP", [])
        bench.wait_samples("cmdDisp.CommandsDispatched", 1)
        send("cmdDisp.CMD_NO_OP", [])
        bench.wait_samples("cmdDisp.CommandsDispatched", 1)
        check("ABS")
        check("cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them",
              lambda h: fth_plan.registry.helpers["channel_tracks_dispatch_count"](h, tick_s=1.0, min_samples=2))
    bench.wait(2.5)  # wait 2 ticks
    send("tlmSend.CLEAR_PACKET_OVERRIDE", ["REALTIME", 61])
