####
# test_dictionary_check.py:
#
# Tests of the dictionary check stage (cmake/target/dictionary_check.cmake) and of the checker registered with it by
# Svc/TlmPacketizer (tools/tlm_packetizer_sizing.py).
####
import json
import platform
import subprocess
import sys

import pytest

from . import cmake
from . import settings

SOURCE = settings.DATA_DIR / "TestDictionaryCheck"
DICTIONARIES = SOURCE / "dictionaries"
SCRIPT = (
    settings.FRAMEWORK_PATH
    / "Svc"
    / "TlmPacketizer"
    / "tools"
    / "tlm_packetizer_sizing.py"
)
OMIT_LIST = "FPRIME_TLM_PACKETIZER_OMIT_LIST"
GENERATED = "TestDictionaryCheckTlmPacketizerSizing.cpp"


def run_script(dictionary, output, *arguments):
    """Run the sizing checker; returns the completed process"""
    return subprocess.run(
        [sys.executable, str(SCRIPT), str(dictionary), "-o", str(output), *arguments],
        capture_output=True,
        text=True,
        check=False,
    )


def write_dictionary(directory, content):
    """Write a dictionary of the given JSON content (or raw text); returns its path"""
    path = directory / "CaseTopologyDictionary.json"
    path.write_text(content if isinstance(content, str) else json.dumps(content))
    return path


def packet_set(members=None, omitted=None, name="Set"):
    """Build a dictionary holding one packet set"""
    return {
        "telemetryPacketSets": [{"name": name, "members": members, "omitted": omitted}]
    }


def test_script_golden(tmp_path):
    """Two sets, a channel in two packets, omitted channels, empty and null packet members"""
    output = tmp_path / "out.cpp"
    result = run_script(DICTIONARIES / "TwoSetsTopologyDictionary.json", output)
    assert result.returncode == 0, result.stderr
    assert result.stderr == ""
    expected = (DICTIONARIES / "TwoSetsTlmPacketizerSizing.cpp").read_text()
    assert output.read_text() == expected


@pytest.mark.parametrize(
    "arguments,channels,detail",
    [
        ([], 5, f"3 listed in packets + 2 omitted; {OMIT_LIST} is ON"),
        (
            ["-D", f"{OMIT_LIST}=ON"],
            5,
            f"3 listed in packets + 2 omitted; {OMIT_LIST} is ON",
        ),
        (
            ["-D", f"{OMIT_LIST}=OFF"],
            3,
            f"3 listed in packets; 2 omitted not counted, {OMIT_LIST} is OFF",
        ),
    ],
)
def test_script_omit_list_variable(tmp_path, arguments, channels, detail):
    """Omitted channels count toward the channels only while the omit list variable is ON"""
    output = tmp_path / "out.cpp"
    result = run_script(
        DICTIONARIES / "TwoSetsTopologyDictionary.json", output, *arguments
    )
    assert result.returncode == 0, result.stderr
    text = output.read_text()
    assert f"static constexpr FwSizeType CHANNELS = {channels};" in text
    assert f"MAX_PACKETIZER_CHANNELS >= {channels} ({detail})" in text
    assert "static constexpr FwSizeType MEMBERSHIPS = 4;" in text


@pytest.mark.parametrize(
    "content",
    [
        {},
        {"telemetryPacketSets": None},
        {"telemetryPacketSets": []},
    ],
)
def test_script_no_packet_sets(tmp_path, content):
    """A dictionary without packet sets gives a source without assertions"""
    output = tmp_path / "out.cpp"
    result = run_script(write_dictionary(tmp_path, content), output)
    assert result.returncode == 0, result.stderr
    text = output.read_text()
    assert '#include "TlmPacketizerConfig/TlmPacketizerCfg.hpp"' in text
    assert "static_assert" not in text


def test_script_null_members_and_omitted(tmp_path):
    """Null members and omitted lists of a packet set are empty"""
    output = tmp_path / "out.cpp"
    result = run_script(write_dictionary(tmp_path, packet_set()), output)
    assert result.returncode == 0, result.stderr
    text = output.read_text()
    assert "static constexpr FwSizeType MEMBERSHIPS = 0;" in text
    assert "static constexpr FwSizeType CHANNELS = 0;" in text


PACKET = {"name": "P", "id": 1, "group": 1, "members": ["a.One"]}


@pytest.mark.parametrize(
    "content,arguments,message",
    [
        ("{ not json", [], "is not valid JSON"),
        ([], [], "dictionary is not a JSON object"),
        ({"telemetryPacketSets": {}}, [], "telemetryPacketSets is not a list"),
        ({"telemetryPacketSets": ["Set"]}, [], "entry that is not an object"),
        ({"telemetryPacketSets": [{"members": []}]}, [], "packet set without a name"),
        (packet_set(members="a.One"), [], "members of packet set Set is not a list"),
        (packet_set(members=["a.One"]), [], "entry that is not an object"),
        (
            packet_set(members=[{"members": "a.One"}]),
            [],
            "members of a packet of packet set Set is not a list",
        ),
        (packet_set(members=[{"members": [1]}]), [], "entry that is not a string"),
        (packet_set(omitted="a.One"), [], "omitted of packet set Set is not a list"),
        (packet_set(omitted=[None]), [], "entry that is not a string"),
        (
            packet_set(members=[PACKET], omitted=["a.One"]),
            [],
            "lists channel a.One in a packet and as omitted",
        ),
        (
            {"telemetryPacketSets": [{"name": "Set"}, {"name": "Set"}]},
            [],
            "packet set name Set is not unique",
        ),
        (packet_set(), ["-D", f"{OMIT_LIST}=MAYBE"], "must be ON or OFF"),
        (packet_set(), ["-D", "OTHER=ON"], "unknown definition OTHER=ON"),
    ],
)
def test_script_failures(tmp_path, content, arguments, message):
    """Each failure exits 1 with a one-line diagnostic and writes no output"""
    output = tmp_path / "out.cpp"
    result = run_script(write_dictionary(tmp_path, content), output, *arguments)
    assert result.returncode == 1
    assert result.stdout == ""
    assert message in result.stderr
    assert len(result.stderr.splitlines()) == 1
    assert not output.exists()
    assert list(tmp_path.iterdir()) == [tmp_path / "CaseTopologyDictionary.json"]


def test_script_unreadable_dictionary(tmp_path):
    """A dictionary that cannot be read exits 1 with a one-line diagnostic and writes no output"""
    output = tmp_path / "out.cpp"
    result = run_script(tmp_path / "MissingTopologyDictionary.json", output)
    assert result.returncode == 1
    assert "cannot read" in result.stderr
    assert len(result.stderr.splitlines()) == 1
    assert not output.exists()


def test_script_failure_keeps_previous_output(tmp_path):
    """A failed run leaves an output of an earlier run untouched"""
    output = tmp_path / "out.cpp"
    output.write_text("previous")
    result = run_script(write_dictionary(tmp_path, "{ not json"), output)
    assert result.returncode == 1
    assert output.read_text() == "previous"


def dictionary_check_build(name, **options):
    """Build the TestDictionaryCheck deployment with the given options"""
    arguments = {
        "FPRIME_FRAMEWORK_PATH": settings.FRAMEWORK_PATH,
        "FPRIME_PROJECT_ROOT": settings.DATA_DIR,
        "FPRIME_PLATFORM": platform.system(),
    }
    arguments.update(options)
    return cmake.get_build(
        name, SOURCE, arguments, make_targets=["TestDictionaryCheck"]
    )


# The deployment's packet set has 3 memberships, 2 channels listed in packets and 3 omitted channels
FITS_BUILD_RESULT = dictionary_check_build("DICTIONARY_CHECK_FITS")
OVER_MEMBERSHIPS_BUILD_RESULT = dictionary_check_build(
    "DICTIONARY_CHECK_OVER_MEMBERSHIPS", TEST_MAX_PACKETIZER_MEMBERSHIPS="2"
)
OVER_CHANNELS_BUILD_RESULT = dictionary_check_build(
    "DICTIONARY_CHECK_OVER_CHANNELS",
    TEST_MAX_PACKETIZER_MEMBERSHIPS="3",
    TEST_MAX_PACKETIZER_CHANNELS="4",
)
OMIT_LIST_OFF_BUILD_RESULT = dictionary_check_build(
    "DICTIONARY_CHECK_OMIT_LIST_OFF",
    TEST_MAX_PACKETIZER_MEMBERSHIPS="3",
    TEST_MAX_PACKETIZER_CHANNELS="4",
    TEST_OMIT_LIST="OFF",
)
NOT_DEPENDENT_BUILD_RESULT = dictionary_check_build(
    "DICTIONARY_CHECK_NOT_DEPENDENT",
    TEST_MAX_PACKETIZER_MEMBERSHIPS="2",
    TEST_NO_TLM_PACKETIZER="ON",
)


def build_output(build):
    """Standard output and error of the deployment's build"""
    return_code, stdout, stderr = build["targets"]["TestDictionaryCheck"]
    return return_code, "".join(stdout) + "".join(stderr)


def test_stage_generates_and_compiles(DICTIONARY_CHECK_FITS):
    """A dependent deployment gets the generated source, compiled into the deployment"""
    cmake.assert_process_success(DICTIONARY_CHECK_FITS)
    generated = DICTIONARY_CHECK_FITS["build"] / GENERATED
    assert generated.exists(), f"Failed to locate {GENERATED} in build output"
    text = generated.read_text()
    assert "static constexpr FwSizeType MEMBERSHIPS = 3;" in text
    assert "static constexpr FwSizeType CHANNELS = 5;" in text
    _, output = build_output(DICTIONARY_CHECK_FITS)
    assert f"{GENERATED}.o" in output


def test_stage_over_memberships_fails(DICTIONARY_CHECK_OVER_MEMBERSHIPS):
    """A packet set with more memberships than MAX_PACKETIZER_MEMBERSHIPS fails the build, naming the set"""
    return_code, output = build_output(DICTIONARY_CHECK_OVER_MEMBERSHIPS)
    assert return_code != 0
    assert "Packet set TestPackets needs MAX_PACKETIZER_MEMBERSHIPS >= 3" in output


def test_stage_over_channels_fails(DICTIONARY_CHECK_OVER_CHANNELS):
    """With the omit list variable ON, omitted channels count toward MAX_PACKETIZER_CHANNELS"""
    return_code, output = build_output(DICTIONARY_CHECK_OVER_CHANNELS)
    assert return_code != 0
    assert "Packet set TestPackets needs MAX_PACKETIZER_CHANNELS >= 5" in output
    assert f"2 listed in packets + 3 omitted; {OMIT_LIST} is ON" in output


def test_stage_omit_list_off_builds(DICTIONARY_CHECK_OMIT_LIST_OFF):
    """With the omit list variable OFF, the dictionary that failed for its omitted channels builds"""
    cmake.assert_process_success(DICTIONARY_CHECK_OMIT_LIST_OFF)
    text = (DICTIONARY_CHECK_OMIT_LIST_OFF["build"] / GENERATED).read_text()
    assert "static constexpr FwSizeType CHANNELS = 2;" in text


def test_stage_skips_deployment_without_owner(DICTIONARY_CHECK_NOT_DEPENDENT):
    """A deployment that does not depend on the checker's module is not checked"""
    cmake.assert_process_success(DICTIONARY_CHECK_NOT_DEPENDENT)
    assert not (DICTIONARY_CHECK_NOT_DEPENDENT["build"] / GENERATED).exists()
