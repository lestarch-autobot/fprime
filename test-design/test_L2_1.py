"""L2-1 — Channel value reaches its first packet (plan.json beside this file is the baseline; this is its stimulus and pass rule).

The named checks declared here are the pass rule of L2-1 (plan amendment 4: the plan entry carries none);
`require` is a completeness precondition (UNVERIFIED), `check` a pass predicate (FAIL). Waits are pacing only; the checks are declared before the stimulus so that a wait that times out leaves a judged window.
"""
import pytest


@pytest.mark.req("TPK-001", "TPK-003")
def test_L2_1(window, send, bench, check, require, fth_plan):
    try:
        with window(29, 30):  # S: setup
            require("STD")
            check("event tlmSend.PacketNotFound count == 0")
            send("tlmSend.OVERRIDE_PACKET_SEND", ["REALTIME", 61, "SEND_NEVER"])
            bench.wait(2.5)  # wait 2 ticks (1 s tick) with half a tick of margin
        with window(30, 31):  # W: measurement
            require("STD")
            check("ABS")
            check("cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them",
                  lambda h: fth_plan.registry.helpers["channel_tracks_dispatch_count"](h, tick_s=1.0, min_samples=2))
            send("cmdDisp.CMD_NO_OP", [])
            bench.wait(2.5)  # wait 2 ticks
            send("cmdDisp.CMD_NO_OP", [])
            bench.wait(2.5)  # wait 2 ticks
    finally:  # T: teardown — the override is cleared whatever the verdict of W, inside a judged window
        with window(31, 32):
            require("STD")
            check("event tlmSend.PacketNotFound count == 0")
            bench.wait(2.5)  # wait 2 ticks
            send("tlmSend.CLEAR_PACKET_OVERRIDE", ["REALTIME", 61])
