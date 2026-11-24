"""L2-2 — Channel value reaches its second packet (plan.json beside this file is the baseline; this is its stimulus).

The checks declared here are, verbatim, the plan's `complete` (require) and `all_of` (check) entries
of L2-2; the plan stays the owner of the pass rule. Waits are pacing only; the checks are declared before the stimulus so that a wait that times out leaves a judged window.
"""
import pytest


@pytest.mark.req("TPK-001", "TPK-003")
def test_L2_2(window, send, bench, check, require, fth_plan):
    send("tlmSend.OVERRIDE_PACKET_SEND", ["REALTIME", 60, "SEND_NEVER"])
    bench.wait(2.5)  # wait 2 ticks (1 s tick) with half a tick of margin
    try:
        with window(40, 41):
            require("STD")
            check("ABS")
            check("cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them",
                  lambda h: fth_plan.registry.helpers["channel_tracks_dispatch_count"](h, tick_s=1.0, min_samples=2))
            send("cmdDisp.CMD_NO_OP", [])
            bench.wait_samples("cmdDisp.CommandsDispatched", 1)
            send("cmdDisp.CMD_NO_OP", [])
            bench.wait_samples("cmdDisp.CommandsDispatched", 1)
    finally:  # the override is cleared whatever the verdict, so the next test starts from the same state
        bench.wait(2.5)  # wait 2 ticks
        send("tlmSend.CLEAR_PACKET_OVERRIDE", ["REALTIME", 60])
