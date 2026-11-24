"""Known-answer tests of the plan helper `channel_tracks_dispatch_count`: one PASS case, every FAIL
branch and the not-judgeable case, on hand-made windows. Run: pytest test/kat (no bench, no UUT)."""
from pathlib import Path
from types import SimpleNamespace as NS

import pytest
from fth.plan import load_plan

HELPER = load_plan(Path(__file__).parent.parent / "int" / "tpk_pool" / "plan.json").registry.helpers["channel_tracks_dispatch_count"]


def ctx(samples, dispatches):
    """samples: [(fsw time, value)]; dispatches: [fsw time of an OpCodeDispatched row]."""
    win = NS(channel=lambda name: [NS(fsw=t, value=v) for t, v in samples],
             events=lambda name: [NS(fsw=t) for t in dispatches])
    return NS(window=win)


def verdict(out):
    return out[0]


def test_pass_each_step_equals_the_dispatches_between_samples():
    # START marker 10.0, NO_OP 10.2 -> sample 11.0 = 5; NO_OP 11.3, END marker 11.6 -> sample 12.0 = 7
    assert verdict(HELPER(ctx([(11.0, 5), (12.0, 7)], [10.0, 10.2, 11.3, 11.6]))) is True


def test_pass_one_sample_when_one_is_asked():
    assert verdict(HELPER(ctx([(11.0, 5)], [10.0, 10.2]), min_samples=1)) is True


def test_fail_no_sample_reached_the_ground():
    assert verdict(HELPER(ctx([], [10.0, 10.2, 11.3, 11.6]))) is False


def test_fail_one_sample_where_two_are_needed():
    assert verdict(HELPER(ctx([(11.0, 5)], [10.0, 10.2, 11.3, 11.6]))) is False


def test_fail_value_does_not_rise():
    assert verdict(HELPER(ctx([(11.0, 5), (12.0, 5)], [10.0, 10.2, 11.3, 11.6]))) is False


def test_fail_step_larger_than_the_dispatches():
    assert verdict(HELPER(ctx([(11.0, 5), (12.0, 9)], [10.0, 10.2, 11.3, 11.6]))) is False


def test_fail_step_smaller_than_the_dispatches():
    # three dispatches strictly between the samples (and more than a tick before the second): step 1 is stale
    assert verdict(HELPER(ctx([(11.0, 5), (14.0, 6)], [10.0, 11.5, 12.0, 12.5]))) is False


def test_not_judgeable_without_dispatch_rows():
    out = HELPER(ctx([(11.0, 5), (12.0, 7)], [10.0]))
    assert out[0] is None and out[3] is True
