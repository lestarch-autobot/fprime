"""Planted faults for the TlmPacketizer membership-pool plan (menu is generated from these)."""
from fth.registry import planted_fault

A_SAMPLE = "cmdDisp.CommandsDispatched samples >= 1 after tlmSend.PacketSent(packetId=60)"
B_SAMPLE = "cmdDisp.CommandsDispatched samples >= 1 after tlmSend.PacketSent(packetId=61)"
B_SENT = "event tlmSend.PacketSent(packetId=61) count == 1"


def _is(command_name, short):
    return command_name == short or command_name.endswith("." + short)


@planted_fault("send_other_packet", applies_to=["L2-1"], fails={"L2-1": [A_SAMPLE]},
               cls="stimulus", mechanism="SEND_PKT is sent for a packet that does not hold the channel (id 62)")
def send_other_packet(test_id, command_name, args):
    if _is(command_name, "tlmSend.SEND_PKT"):
        return [62, args[1]]
    return None


@planted_fault("second_packet_left_disabled", applies_to=["L2-2"], fails={"L2-2": [B_SENT, B_SAMPLE]},
               cls="uut-state", kind="insert",
               mechanism="the realtime section is disabled before SEND_PKT, so the UUT holds the packet back")
def second_packet_left_disabled(test_id, command_name, args):
    if _is(command_name, "tlmSend.SEND_PKT"):
        return [("tlmSend.ENABLE_SECTION", ["REALTIME", "DISABLED"]), (command_name, args)]
    return None
