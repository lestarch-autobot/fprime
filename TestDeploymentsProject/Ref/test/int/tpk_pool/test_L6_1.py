"""L6-1 — Unknown packet is refused (plan.json beside this file is the baseline; this is its stimulus).

The checks declared here are, verbatim, the plan's `complete` (require) and `all_of` (check) entries
of L6-1; the plan stays the owner of the pass rule. Waits are pacing only.
"""


def test_L6_1(window, send, bench, check, require):
    with window(50, 51):
        require("both markers present")
        require("commands_sent == count(event cmdDisp.OpCodeDispatched)")
        # A refusal answers VALIDATION_ERROR (OpCodeError, no OpCodeCompleted): pace on either outcome.
        send("tlmSend.SEND_PKT", [999, "REALTIME"], wait_completion=False)
        bench.wait_any_event(["cmdDisp.OpCodeError", "cmdDisp.OpCodeCompleted"])
        check("event cmdDisp.OpCodeError count == 1")
        check("event tlmSend.PacketNotFound(packetId=999) count == 1")
        check("event tlmSend.PacketSent count == 0")
        check("event tlmSend.NoChan count == 0")
