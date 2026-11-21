"""Every event pattern of the plan parses a real line of its event (test/kat/event_lines.json).

The lines are event texts as the ground system prints them: taken from a recorded run, or written from the
format string of SeqScheduler.fpp for events no clean run emits. A pattern that no longer fits its event gives
the event no arguments, and every check naming an argument then counts nothing.
"""
import json
import pathlib
import re

import pytest

HERE = pathlib.Path(__file__).resolve().parent
PATTERNS = json.loads((HERE.parent / "int" / "plan.json").read_text(encoding="utf-8"))["event_patterns"]
LINES = json.loads((HERE / "event_lines.json").read_text(encoding="utf-8"))


def test_every_pattern_has_a_line():
    assert sorted(PATTERNS) == sorted(LINES)
    assert all(LINES[name] for name in PATTERNS)


@pytest.mark.parametrize("name", sorted(PATTERNS))
def test_pattern_parses_its_event(name):
    pattern = re.compile(PATTERNS[name])
    for line in LINES[name]:
        found = pattern.search(line)
        assert found is not None, f"{name}: {PATTERNS[name]!r} does not match {line!r}"
        assert all(value is not None for value in found.groupdict().values()), f"{name}: unset group in {line!r}"


def test_skip_cause_is_parsed_without_its_enumeration_name():
    for name in ("seqSched.RowSkippedDisabled", "seqSchedB.RowSkippedDisabled"):
        found = re.search(PATTERNS[name], "Row 3 skipped: SeqScheduler_SkipCause.WAITING_AT_DISABLE")
        assert found.groupdict() == {"row": "3", "cause": "WAITING_AT_DISABLE"}
        assert re.search("Row (?P<row>\\d+) skipped: disabled$", "Row 3 skipped: ROW_DISABLED") is None  # the old pattern
