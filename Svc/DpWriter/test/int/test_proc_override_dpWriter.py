"""test_proc_override_dpWriter.py:

Reusable integration tests for the DpWriter processing override (SVC-DPWRITER-009, -010, -011): while
ENABLED, the SET_PROC_OVERRIDE mask replaces the processing types requested by every data product (a
clear bit forces that processing off); DISABLED restores header-controlled processing. Masks that
select a nonexistent or unconnected processing port are rejected, and the override is reported in
events and telemetry.

Producing a data product is deployment specific, so the producer command is read from the deployment
configuration file given with --deployment-config, for example:

    "Svc.DpWriter.producer.command": "Ref.dpDemo.Dp",
    "Svc.DpWriter.producer.args": ["IMMEDIATE", 10, "PROC_TYPE_ZLIB_DEFLATE"],
    "Svc.DpWriter.producer.bytes": 1535

The command must request PROC_TYPE_ZLIB_DEFLATE processing and result in exactly one data product file
written by `Svc.DpWriter` within a few seconds. The forced-off test is skipped when no producer is
configured. The producer command is only ever sent while the override forces processing off: in a
deployment that does not connect the zlib processing port (such as Ref), sending it by hand or from
another test asserts the flight software (nasa/fprime#6346).
"""

import pytest

PRODUCER_COMMAND_KEY = "Svc.DpWriter.producer.command"
PRODUCER_ARGS_KEY = "Svc.DpWriter.producer.args"
PRODUCER_BYTES_KEY = "Svc.DpWriter.producer.bytes"

# Bit of Fw.DpCfg.ProcType.PROC_TYPE_ZLIB_DEFLATE, the processing type the producer requests
ZLIB_DEFLATE_MASK = 0x01

# An ENABLED override with no bits set forces every processing type off
NO_PROCESSING_MASK = 0x00

# A mask selecting every bit: it is rejected unless a deployment connects all eight processing ports
ALL_BITS_MASK = 0xFF

# Seconds allowed for the deployment to write a product
WRITE_TIMEOUT = 20


def dp_writer(fprime_test_api, name):
    """Qualified mnemonic of a DpWriter command, event, or channel"""
    return fprime_test_api.get_mnemonic("Svc.DpWriter") + "." + name


class ProcOverride:
    """Drives SET_PROC_OVERRIDE and tracks the override in effect

    Both telemetry channels update on change, so a channel is awaited only when the command changes it.
    """

    DEFAULT = ("DISABLED", 0)

    def __init__(self, fprime_test_api):
        self.api = fprime_test_api
        self.current = self.DEFAULT
        # Set when a product that needs the override may still be in flight, so teardown must not clear it
        self.hold = False

    def set(self, state, mask):
        """Send SET_PROC_OVERRIDE and verify the OK response, event, and changed telemetry"""
        event_start = self.api.get_event_test_history().size()
        tlm_start = self.api.get_telemetry_test_history().size()
        self.api.send_and_assert_command(
            dp_writer(self.api, "SET_PROC_OVERRIDE"), [state, mask], max_delay=10
        )
        self.api.assert_event(
            dp_writer(self.api, "ProcOverrideSet"), [state, mask], start=event_start, timeout=10
        )
        if state != self.current[0]:
            self.api.assert_telemetry(
                dp_writer(self.api, "ProcOverrideState"), state, start=tlm_start, timeout=10
            )
        if mask != self.current[1]:
            self.api.assert_telemetry(
                dp_writer(self.api, "ProcOverrideMask"), mask, start=tlm_start, timeout=10
            )
        self.current = (state, mask)

    def clear(self):
        """Return DpWriter to header-controlled processing"""
        self.set(*self.DEFAULT)


@pytest.fixture
def proc_override(fprime_test_api):
    """Drive the override from its default and clear it after each test so other tests see default behavior"""
    override = ProcOverride(fprime_test_api)
    yield override
    if override.hold:
        fprime_test_api.log(
            "WARNING: leaving the processing override in place; a product that needs it may still be in flight"
        )
    else:
        override.clear()


def test_proc_override_set_and_clear(fprime_test_api, proc_override):
    """An empty ENABLED override is accepted, a DISABLED mask is accepted even when its port is unconnected, and DISABLED clears it"""
    proc_override.set("ENABLED", NO_PROCESSING_MASK)
    proc_override.set("DISABLED", ZLIB_DEFLATE_MASK)
    proc_override.clear()


def test_proc_override_rejects_unavailable_ports(fprime_test_api, proc_override):
    """Forcing processing on through a nonexistent or unconnected port is rejected without changing the override"""
    start = fprime_test_api.get_event_test_history().size()
    fprime_test_api.send_command(
        dp_writer(fprime_test_api, "SET_PROC_OVERRIDE"), ["ENABLED", ALL_BITS_MASK]
    )
    # A deployment connecting all eight processing ports legitimately accepts every bit
    try:
        fprime_test_api.assert_event(
            dp_writer(fprime_test_api, "ProcOverrideSet"), start=start, timeout=2
        )
    except AssertionError:
        pass
    else:
        proc_override.current = ("ENABLED", ALL_BITS_MASK)
        pytest.skip("deployment connects every processing port; nothing is unavailable")
    invalid = fprime_test_api.assert_event(
        dp_writer(fprime_test_api, "ProcOverrideInvalid"), start=start, timeout=10
    )
    # ProcOverrideInvalid(state, procTypes, allowed): the rejected mask must exceed the allowed mask
    args = invalid.get_args()
    assert args[1].val == ALL_BITS_MASK
    assert (ALL_BITS_MASK & ~args[2].val) != 0
    fprime_test_api.assert_event(
        fprime_test_api.get_mnemonic("Svc.CommandDispatcher") + "." + "OpCodeError",
        start=start,
        timeout=10,
    )
    # The override is unchanged, so no ProcOverrideSet event was emitted
    fprime_test_api.assert_event_count(0, dp_writer(fprime_test_api, "ProcOverrideSet"), start=start)


def test_proc_override_forces_processing_off(fprime_test_api, proc_override):
    """A product requesting zlib processing is written while the override forces that type off

    The product is not routed to the zlib processing port. When the deployment configures the unprocessed
    size of the product, the written size is checked against it; otherwise only the write is checked.
    """
    config = fprime_test_api.load_config_file() or {}
    command = config.get(PRODUCER_COMMAND_KEY)
    if not command:
        pytest.skip(f"{PRODUCER_COMMAND_KEY} not set in the deployment configuration")
    args = config.get(PRODUCER_ARGS_KEY, [])
    proc_override.set("ENABLED", NO_PROCESSING_MASK)
    start = fprime_test_api.get_event_test_history().size()
    # Until the product is written, clearing the override would route it to the unconnected port
    proc_override.hold = True
    fprime_test_api.send_and_assert_command(command, args, max_delay=10)
    written = fprime_test_api.assert_event(
        dp_writer(fprime_test_api, "FileWritten"), start=start, timeout=WRITE_TIMEOUT
    )
    proc_override.hold = False
    expected_bytes = config.get(PRODUCER_BYTES_KEY)
    if expected_bytes is not None:
        assert written.get_args()[0].val == expected_bytes
