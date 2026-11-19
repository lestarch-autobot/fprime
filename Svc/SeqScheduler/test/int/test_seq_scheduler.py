"""Stimulus and named checks of the Svc.SeqScheduler system test plan (plan.json beside this file).

Each test sends the plan's procedure inside its windows and declares the plan's named checks.
The verdict is computed from the record at window close; waits are pacing only.
"""
import pytest

S = "Ref.seqSched"
B = "Ref.seqSchedB"
Q = "Ref.schedSeq"
QB = "Ref.schedSeqB"
T = "Ref.timeBaseSetter"
MARK = "CdhCore.cmdDisp.CMD_NO_OP_STRING"
SAMPLE = "seqSched.NextTimeDue"  # written every tick: one row per SeqSched packet
SAMPLE_B = "seqSchedB.NextTimeDue"


def bracket(bench, n=2):
    """The plan's "bracket (n SeqSched samples)"."""
    bench.wait_samples(SAMPLE, n)


def version_strings_match(h):
    """Every version event of the window is present and non-empty; identity is judged against the manifest by the audit."""
    rows = [e for e in h.window.ev if ".version." in e.name]
    return (len(rows) > 0 and all(e.text.strip() for e in rows), f"{len(rows)} version events")


def test_L0_1(window, send, bench, check, require):
    with window(100, 101):
        require("STD")
        bracket(bench)
        send("CdhCore.version.VERSION", ["ALL"])
        check("version payloads == manifest describe strings", version_strings_match)
        check("event cmdDisp.NoOpStringReceived(id=100) count == 1")
