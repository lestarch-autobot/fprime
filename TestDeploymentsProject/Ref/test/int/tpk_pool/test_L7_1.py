"""L7-1 — Final state (plan.json beside this file is the baseline; this is its stimulus and pass rule).

The named checks declared here are the pass rule of L7-1 (plan amendment 4: the plan entry carries none);
`require` is a completeness precondition (UNVERIFIED), `check` a pass predicate (FAIL). Waits are pacing only; the checks are declared before the stimulus so that a wait that times out leaves a judged window.
"""


def test_L7_1(window, send, bench, check, require, fth_plan):
    with window(59, 60):  # S: setup
        require("STD")
        check("event tlmSend.PacketNotFound count == 0")
        send("tlmSend.CLEAR_PACKET_OVERRIDE", ["REALTIME", 60])
        send("tlmSend.CLEAR_PACKET_OVERRIDE", ["REALTIME", 61])
        bench.wait(2.5)  # wait 2 ticks (1 s tick) with half a tick of margin
    with window(60, 61):  # W: measurement
        require("STD")
        check("ABS")
        check("cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them",
              lambda h: fth_plan.registry.helpers["channel_tracks_dispatch_count"](h, tick_s=1.0, min_samples=2))
        send("cmdDisp.CMD_NO_OP", [])
        bench.wait_samples("cmdDisp.CommandsDispatched", 1)
        send("cmdDisp.CMD_NO_OP", [])
        bench.wait_samples("cmdDisp.CommandsDispatched", 1)
