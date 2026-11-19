"""SeqScheduler system test L0-1 (plan.json)."""
import pytest

from seqsched_common import *  # noqa: F401,F403


def test_L0_1(window, send, bench, check, require):
    with window(100, 101):
        require("STD")
        bracket(bench)
        bench.action("manifest_versions")
        send("CdhCore.version.VERSION", ["ALL"])
        check("version payloads == manifest describe strings", version_strings_match)
        check("event cmdDisp.NoOpStringReceived(id=100) count == 1")
