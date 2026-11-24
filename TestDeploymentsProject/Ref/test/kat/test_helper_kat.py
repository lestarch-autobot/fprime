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


# The rule `within_ticks` (plan amendment 6): every dispatch at least that many ticks before the last one in the
# window has a sample of its own within that many ticks after it. Window of L2-1, L2-2, L7-1 with three-tick waits:
# START marker 10.0 -> sample 10.9 = 5; NO_OP 10.5 (same tick), then NO_OP 14.2 -> sample 14.9; END marker 17.8.
NORMAL = [10.0, 10.5, 14.2, 17.8]


def strict(samples, dispatches=NORMAL):
    return verdict(HELPER(ctx(samples, dispatches), tick_s=1.0, min_samples=2, within_ticks=2))


def test_within_pass_three_samples_one_per_command():
    assert strict([(10.4, 4), (10.9, 5), (14.9, 6)]) is True


def test_within_fail_sample_of_the_second_command_lost():
    # the count is still two and the one step is right: only the new rule sees it
    assert verdict(HELPER(ctx([(10.4, 4), (10.9, 5)], NORMAL))) is True
    assert strict([(10.4, 4), (10.9, 5)]) is False


def test_within_fail_sample_of_the_first_command_lost_merged_step_of_two():
    assert verdict(HELPER(ctx([(10.4, 4), (14.9, 6)], NORMAL))) is True
    assert strict([(10.4, 4), (14.9, 6)]) is False


def test_within_fail_sample_written_more_than_two_ticks_late():
    assert verdict(HELPER(ctx([(10.4, 4), (10.9, 5), (16.3, 6)], NORMAL))) is True
    assert strict([(10.4, 4), (10.9, 5), (16.3, 6)]) is False


def test_within_pass_sample_exactly_two_ticks_after_the_dispatch():
    assert strict([(10.4, 4), (10.9, 5), (16.2, 6)]) is True


def test_within_end_marker_dispatch_needs_no_sample():
    # nothing after 17.8 and nothing within two ticks before it: the last dispatch is exempt
    assert strict([(10.4, 4), (10.9, 5), (14.9, 6)], [10.0, 10.5, 14.2, 17.8]) is True


def test_within_off_by_default_keeps_the_one_sample_check():
    assert verdict(HELPER(ctx([(10.9, 5)], [10.0, 10.5, 14.2]), min_samples=1)) is True


# Two packets carrying the channel (TpkA and TpkB, both enabled in L0-2 and L7-1). Each change is downlinked once
# per packet with one time tag; the recorder writes those lines one after the other, and the tool's export
# collapses adjacent identical lines (`fth/export.py`, `dedup_adjacent`), so the window holds ONE sample per
# change. The helper itself does not collapse equal samples: two equal rows are a step of 0 and FAIL.
RAW = {  # one line of each venue's recorder, value and time as placeholders
    "ref": "2026-11-24T21:58:{s:02d}.068465,(2(0)-17955575{s:02d}:68465),CdhCore.cmdDisp.CommandsDispatched,16777216,{v}",
    "pico2": "TB_NONE: {s}.290400s, context=0,(0(0)-{s}:290400),CdhCore.cmdDisp.CommandsDispatched,16777216,{v}",
}
OTHER = {
    "ref": "2026-11-24T21:58:{s:02d}.068525,(2(0)-17955575{s:02d}:68525),FileHandling.fileUplink.FilesReceived,1,0",
    "pico2": "TB_NONE: {s}.290500s, context=0,(0(0)-{s}:290500),FileHandling.fileUplink.FilesReceived,1,0",
}


def exported(tmp_path, lines):
    """The CommandsDispatched rows the tool's export makes of a recorder channel.log: [(fsw, value)]."""
    import json
    from fth.export import export
    rec = tmp_path / "raw" / "fprime-cli-kat"
    rec.mkdir(parents=True)
    (rec / "channel.log").write_text("".join(line + "\n" for line in lines))
    export(tmp_path / "raw", tmp_path / "record")
    rows = [json.loads(r) for r in (tmp_path / "record" / "channels.jsonl").read_text().splitlines()]
    return [(r["fsw"], r["value"]) for r in rows if r["name"].endswith("cmdDisp.CommandsDispatched")]


@pytest.mark.parametrize("venue", sorted(RAW))
def test_two_packets_in_one_tick_give_one_sample_and_pass(tmp_path, venue):
    # each change arrives in packet 60 and in packet 61, two recorder lines each: four adjacent identical lines
    lines = [RAW[venue].format(s=s, v=v) for s, v in ((11, 5), (12, 6), (13, 7)) for _ in range(4)]
    samples = exported(tmp_path, lines)
    assert [v for _, v in samples] == [5, 6, 7]
    t = samples[0][0]
    assert verdict(HELPER(ctx(samples, [t - 0.5, t + 0.5, t + 1.5]))) is True
    assert verdict(HELPER(ctx(samples, [t - 0.5, t + 0.5, t + 1.5, t + 4.5]), within_ticks=2)) is True


@pytest.mark.parametrize("venue", sorted(RAW))
def test_one_packet_arriving_apart_from_the_other_gives_two_equal_samples(tmp_path, venue):
    # the second packet's lines come after another channel's line (a held packet released later with its old
    # time tag, as in the teardown windows of L2-1 and L2-2): the export keeps both rows
    lines = [RAW[venue].format(s=11, v=5)] * 2 + [OTHER[venue].format(s=11)] + [RAW[venue].format(s=11, v=5)] * 2
    assert [v for _, v in exported(tmp_path, lines)] == [5, 5]


def test_fail_two_equal_samples_are_not_collapsed_by_the_helper():
    # pins the rule: equal samples in a judged window are a step of 0, so a second copy of a sample FAILS
    out = HELPER(ctx([(11.0, 5), (11.0, 5), (12.0, 6)], [10.5, 11.5, 12.5]))
    assert out[0] is False and "5->5" in out[1]


# The slack of one telemetry tick on either edge of a step (approved rule, pinned here as it is).
def test_slack_lower_edge_counts_a_dispatch_up_to_one_tick_before_the_earlier_sample():
    # no dispatch between the two samples; the one at 10.2, 0.7 s before the first sample, is counted again
    assert verdict(HELPER(ctx([(10.9, 5), (11.5, 6)], [10.2, 13.0]))) is True


def test_slack_lower_edge_ends_one_tick_before_the_earlier_sample():
    assert verdict(HELPER(ctx([(10.9, 5), (11.5, 6)], [9.9, 13.0]))) is False


def test_slack_upper_edge_a_dispatch_in_the_last_tick_before_a_sample_may_be_missing_from_it():
    # dispatches at 11.0 and 11.8; the sample at 12.0 may hold one or both
    assert verdict(HELPER(ctx([(10.9, 5), (12.0, 6)], [10.2, 11.0, 11.8]))) is True
    assert verdict(HELPER(ctx([(10.9, 5), (12.0, 7)], [10.2, 11.0, 11.8]))) is True


def test_first_sample_value_is_not_tied_to_anything():
    # only steps are judged: the absolute count is not (the image starts anew at every run, the count does not)
    assert verdict(HELPER(ctx([(11.0, 500), (12.0, 501)], [10.2, 11.3]))) is True
