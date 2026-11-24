"""L6-1 — Unknown packet is refused (plan.json beside this file is the baseline; this is its stimulus and pass rule).

The named checks declared here are the pass rule of L6-1 (plan amendment 4: the plan entry carries none);
`require` is a completeness precondition (UNVERIFIED), `check` a pass predicate (FAIL). Waits are pacing only; the checks are declared before the stimulus so that a wait that times out leaves a judged window.
"""


def test_L6_1(window, send, bench, check, require):
    with window(50, 51):
        require("both markers present")
        require("commands_sent == count(event cmdDisp.OpCodeDispatched)")
        check("event cmdDisp.OpCodeError count == 1")
        check("event tlmSend.PacketNotFound(packetId=999) count == 1")
        check("event tlmSend.PacketSent count == 0")
        check("event tlmSend.NoChan count == 0")
        # A refusal answers VALIDATION_ERROR (OpCodeError, no OpCodeCompleted): pace on either outcome.
        send("tlmSend.SEND_PKT", [999, "REALTIME"], wait_completion=False)
        bench.wait_any_event(["cmdDisp.OpCodeError", "cmdDisp.OpCodeCompleted"])
