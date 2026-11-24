"""Check helper and planted faults for the TlmPacketizer membership-pool plan (menu is generated from these)."""
from fth.registry import helper, planted_fault

CH = "cmdDisp.CommandsDispatched"
VALUE = "cmdDisp.CommandsDispatched tracks cmdDisp.OpCodeDispatched: >= 2 increasing samples, each step == dispatches between them"
NOTFOUND = "event tlmSend.PacketNotFound(packetId=999) count == 1"
OPERR = "event cmdDisp.OpCodeError count == 1"


@helper("channel_tracks_dispatch_count")
def channel_tracks_dispatch_count(ctx, tick_s=1.0, min_samples=2):
    """The channel's samples in the window rise, and each rise equals the number of OpCodeDispatched
    rows the archive holds between the two samples (one telemetry tick of slack on either edge)."""
    samples = ctx.window.channel(CH)
    disp = [e.fsw for e in ctx.window.events("cmdDisp.OpCodeDispatched")]
    if len(disp) < min_samples:
        return None, f"only {len(disp)} dispatch rows in the window", [], True
    if len(samples) < min_samples:
        return False, f"{len(samples)} samples, need {min_samples}"
    for a, b in zip(samples, samples[1:]):
        step = b.value - a.value
        lo = sum(1 for t in disp if a.fsw < t <= b.fsw - tick_s)
        hi = sum(1 for t in disp if a.fsw - tick_s < t <= b.fsw)
        if step < 1 or not lo <= step <= hi:
            return False, f"step {a.value}->{b.value} outside [{max(lo, 1)}, {hi}]"
    return True, f"{len(samples)} samples {samples[0].value}..{samples[-1].value}"


def _is(command_name, short):
    return command_name == short or command_name.endswith("." + short)


def _hold_back(packet):
    def fault(test_id, command_name, args):
        if _is(command_name, "tlmSend.OVERRIDE_PACKET_SEND"):
            return [(command_name, args), (command_name, [args[0], packet, "SEND_NEVER"])]
        return None
    return fault


planted_fault("first_packet_also_held", applies_to=["L2-1"], fails={"L2-1": [VALUE]}, cls="uut-state", kind="insert",
              mechanism="packet 60 is also set SEND_NEVER, so no packet downlinks the channel")(_hold_back(60))
planted_fault("second_packet_also_held", applies_to=["L2-2"], fails={"L2-2": [VALUE]}, cls="uut-state", kind="insert",
              mechanism="packet 61 is also set SEND_NEVER, so no packet downlinks the channel")(_hold_back(61))


@planted_fault("known_packet_requested", applies_to=["L6-1"], fails={"L6-1": [NOTFOUND, OPERR]}, cls="stimulus",
               mechanism="SEND_PKT is sent for packet 60, which exists, so no refusal is produced")
def known_packet_requested(test_id, command_name, args):
    if _is(command_name, "tlmSend.SEND_PKT"):
        return [60, args[1]]
    return None


@planted_fault("final_packets_held", applies_to=["L7-1"], fails={"L7-1": [VALUE]}, cls="uut-state", kind="insert",
               mechanism="both test packets are set SEND_NEVER after the overrides are cleared, so the channel is not downlinked")
def final_packets_held(test_id, command_name, args):
    if _is(command_name, "tlmSend.CLEAR_PACKET_OVERRIDE") and int(args[1]) == 61:
        o = command_name.replace("CLEAR_PACKET_OVERRIDE", "OVERRIDE_PACKET_SEND")
        return [(command_name, args), (o, [args[0], 60, "SEND_NEVER"]), (o, [args[0], 61, "SEND_NEVER"])]
    return None
