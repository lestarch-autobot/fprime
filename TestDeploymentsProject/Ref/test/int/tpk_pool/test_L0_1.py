"""L0-1 — Image identity (plan.json beside this file is the baseline; this is its stimulus).

The checks declared here are, verbatim, the plan's `complete` (require) and `all_of` (check) entries
of L0-1; the plan stays the owner of the pass rule. Waits are pacing only; the checks are declared before the stimulus so that a wait that times out leaves a judged window.
"""


def test_L0_1(window, send, check, require):
    with window(10, 11):
        require("STD")
        check("event version.FrameworkVersion count >= 1")
        send("version.VERSION", ["ALL"])
