"""L0-2 — Packets flow on change (plan.json beside this file is the baseline; this is its stimulus).

The checks declared here are, verbatim, the plan's `complete` (require) and `all_of` (check) entries
of L0-2; the plan stays the owner of the pass rule. Waits are pacing only.
"""


def test_L0_2(window, send, bench, check, require, fth_plan):
    with window(20, 21):
        require("STD")
        send("cmdDisp.CMD_NO_OP", [])
        bench.wait_samples("cmdDisp.CommandsDispatched", 1)
        check("ABS")
        check("cmdDisp.CommandsDispatched has >= 1 sample in the window",
              lambda h: fth_plan.registry.helpers["channel_tracks_dispatch_count"](h, tick_s=1.0, min_samples=1))
