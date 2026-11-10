"""test_BufferAccumulator.py:

Reusable integration tests for the Svc.BufferAccumulator drain limit (pipelined draining).

The drain-limit command, its events and telemetry are exercised directly. The pipelined-drain test
also needs buffers to flow through the accumulator; producing them is deployment specific, so it
reuses the data-product producer configured for the DpCatalog tests in the deployment configuration
given with --deployment-config, for example:

    "Svc.BufferAccumulator": "DataProducts.dpBufferAccumulator",
    "Svc.DpCatalog.producer.command": "Ref.dpDemo.Dp",
    "Svc.DpCatalog.producer.args": ["IMMEDIATE", 10, "PROC_TYPE_NONE"]

Each producer command must result in exactly one buffer reaching the accumulator (within
TELEMETRY_TIMEOUT seconds) and, once drained, one Svc.DpWriter FileWritten event. The pipelined-drain test is skipped when no producer is configured.
"""

import pytest

PRODUCER_COMMAND_KEY = "Svc.DpCatalog.producer.command"
PRODUCER_ARGS_KEY = "Svc.DpCatalog.producer.args"

# Seconds allowed for a command's events, and for telemetry to come down on the rate group
EVENT_TIMEOUT = 10
TELEMETRY_TIMEOUT = 20
# Seconds allowed for the consumer to process a batch of drained buffers
DRAIN_TIMEOUT = 30


def accumulator(fprime_test_api, name):
    """Qualified mnemonic of a BufferAccumulator command, event, or channel"""
    return fprime_test_api.get_mnemonic("Svc.BufferAccumulator") + "." + name


def set_drain_limit(fprime_test_api, limit):
    """Set the drain limit and check the response, event, and telemetry (REQ-BA-DL-003/012/018)"""
    start = fprime_test_api.get_event_test_history().size()
    tlm_start = fprime_test_api.get_telemetry_test_history().size()
    fprime_test_api.send_and_assert_command(
        accumulator(fprime_test_api, "BA_SetDrainLimit"),
        [limit],
        max_delay=EVENT_TIMEOUT,
    )
    fprime_test_api.assert_event(
        accumulator(fprime_test_api, "BA_DrainLimitSet"),
        [limit],
        start=start,
        timeout=EVENT_TIMEOUT,
    )
    fprime_test_api.assert_telemetry(
        accumulator(fprime_test_api, "BA_DrainLimit"),
        limit,
        start=tlm_start,
        timeout=TELEMETRY_TIMEOUT,
    )


def reject_drain_limit(fprime_test_api, limit):
    """Send an out-of-range drain limit and return the maximum the component reports (REQ-BA-DL-004)"""
    start = fprime_test_api.get_event_test_history().size()
    fprime_test_api.send_command(
        accumulator(fprime_test_api, "BA_SetDrainLimit"), [limit]
    )
    rejected = fprime_test_api.assert_event(
        accumulator(fprime_test_api, "BA_InvalidDrainLimit"),
        start=start,
        timeout=EVENT_TIMEOUT,
    )
    assert (
        rejected.get_args()[0].val == limit
    ), "BA_InvalidDrainLimit must report the rejected value"
    op_code_error = fprime_test_api.assert_event(
        fprime_test_api.get_mnemonic("Svc.CommandDispatcher") + ".OpCodeError",
        start=start,
        timeout=EVENT_TIMEOUT,
    )
    assert "VALIDATION_ERROR" in str(
        op_code_error.get_args()[1].val
    ), "Invalid limit must fail validation"
    return rejected.get_args()[1].val


def producer(fprime_test_api):
    """Return the configured (command, args) that sends one buffer to the accumulator, or skip"""
    config = fprime_test_api.load_config_file() or {}
    command = config.get(PRODUCER_COMMAND_KEY)
    if not command:
        pytest.skip(f"{PRODUCER_COMMAND_KEY} not set in the deployment configuration")
    return command, config.get(PRODUCER_ARGS_KEY, [])


def test_set_drain_limit(fprime_test_api):
    """A drain limit in range is accepted, announced, and reported in telemetry

    Covers: REQ-BA-DL-003, REQ-BA-DL-005, REQ-BA-DL-012, REQ-BA-DL-018
    """
    try:
        set_drain_limit(fprime_test_api, 2)
    finally:
        set_drain_limit(fprime_test_api, 1)


def test_set_drain_limit_invalid(fprime_test_api):
    """Zero and values above the queue depth are rejected with VALIDATION_ERROR and leave the limit alone

    Covers: REQ-BA-DL-004
    """
    tlm_start = fprime_test_api.get_telemetry_test_history().size()
    max_allowed = reject_drain_limit(fprime_test_api, 0)
    assert (
        max_allowed >= 1
    ), "The reported maximum must admit at least one buffer in flight"
    reject_drain_limit(fprime_test_api, max_allowed + 1)
    # No BA_DrainLimit update accompanies a rejection
    assert (
        fprime_test_api.await_telemetry(
            accumulator(fprime_test_api, "BA_DrainLimit"), start=tlm_start, timeout=2
        )
        is None
    ), "A rejected BA_SetDrainLimit must not change the drain limit"
    # The reported maximum itself is accepted
    try:
        set_drain_limit(fprime_test_api, max_allowed)
    finally:
        set_drain_limit(fprime_test_api, 1)


def test_pipelined_drain(fprime_test_api):
    """Accumulated buffers drain through a window wider than one and all return

    The window itself (REQ-BA-DL-007) is verified by unit test: TlmChan downlinks only the
    latest value of a channel per tick, so the peak outstanding count is not observable here.

    Covers: REQ-BA-DL-006, REQ-BA-DL-008, REQ-BA-DL-011
    """
    command, args = producer(fprime_test_api)
    num_buffers = 3
    mode = accumulator(fprime_test_api, "BA_SetMode")
    outstanding = accumulator(fprime_test_api, "BA_NumOutstandingBuffers")
    queued = accumulator(fprime_test_api, "BA_NumQueuedBuffers")
    file_written = fprime_test_api.get_mnemonic("Svc.DpWriter") + ".FileWritten"
    try:
        fprime_test_api.send_and_assert_command(
            mode, ["ACCUMULATE"], max_delay=EVENT_TIMEOUT
        )
        # Produce one buffer at a time: the producer may not accept overlapping requests
        for count in range(1, num_buffers + 1):
            tlm_start = fprime_test_api.get_telemetry_test_history().size()
            fprime_test_api.send_and_assert_command(
                command, args, max_delay=EVENT_TIMEOUT
            )
            fprime_test_api.assert_telemetry(
                queued, count, start=tlm_start, timeout=TELEMETRY_TIMEOUT
            )

        set_drain_limit(fprime_test_api, num_buffers)
        start = fprime_test_api.get_event_test_history().size()
        tlm_start = fprime_test_api.get_telemetry_test_history().size()
        fprime_test_api.send_and_assert_command(
            mode, ["DRAIN"], max_delay=EVENT_TIMEOUT
        )
        fprime_test_api.assert_event_count(
            num_buffers, file_written, start=start, timeout=DRAIN_TIMEOUT
        )
        fprime_test_api.assert_telemetry(
            queued, 0, start=tlm_start, timeout=TELEMETRY_TIMEOUT
        )
        fprime_test_api.assert_telemetry(
            outstanding, 0, start=tlm_start, timeout=TELEMETRY_TIMEOUT
        )
    finally:
        fprime_test_api.send_and_assert_command(
            mode, ["DRAIN"], max_delay=EVENT_TIMEOUT
        )
        set_drain_limit(fprime_test_api, 1)
