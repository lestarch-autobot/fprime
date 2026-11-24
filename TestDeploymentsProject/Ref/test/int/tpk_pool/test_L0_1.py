"""L0-1 — Image identity (plan.json beside this file is the baseline; this is its stimulus and pass rule).

The named checks declared here are the pass rule of L0-1 (plan amendment 4: the plan entry carries none);
`require` is a completeness precondition (UNVERIFIED), `check` a pass predicate (FAIL). Waits are pacing only; the checks are declared before the stimulus so that a wait that times out leaves a judged window.
"""


def test_L0_1(window, send, check, require):
    with window(10, 11):
        require("STD")
        check("event version.FrameworkVersion count >= 1")
        send("version.VERSION", ["ALL"])
