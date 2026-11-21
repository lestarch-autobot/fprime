"""Known-answer tests of scripts/seq_schedule_table.py."""
import pathlib
import struct
import sys
import zlib

import pytest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / "scripts"))
import seq_schedule_table as table  # noqa: E402

NAME_SIZE = 240
ROW = {"timeBase": "TB_WORKSTATION_TIME", "seconds": 1800000000, "useconds": 5, "mode": "ABSOLUTE",
       "busyPolicy": "CANCEL_RUNNING", "enabled": True, "fileName": "/seq/a.bin"}
EMPTY = {"timeBase": "TB_NONE", "seconds": 0, "useconds": 0, "mode": "ABSOLUTE",
         "busyPolicy": "WAIT_FOR_RUNNING", "enabled": False, "fileName": ""}


def resealed(data):
    body = data[: -table.DIGEST.size]
    return body + table.DIGEST.pack(zlib.crc32(body) & 0xFFFFFFFF)


def test_round_trip_and_defaults():
    data = table.pack_table([ROW, {}, {"seconds": 7, "mode": "START_RELATIVE", "fileName": "b.bin"}], NAME_SIZE)
    rows = table.unpack_table(data, NAME_SIZE)
    assert rows[0] == ROW
    assert rows[1] == EMPTY
    # "enabled" defaults to "has a file name"
    assert rows[2] == {"timeBase": "TB_NONE", "seconds": 7, "useconds": 0, "mode": "START_RELATIVE",
                       "busyPolicy": "WAIT_FOR_RUNNING", "enabled": True, "fileName": "b.bin"}
    assert len(data) == table.HEADER.size + 3 * (table.ROW_HEAD.size + NAME_SIZE + 1) + table.DIGEST.size


def test_layout_is_the_one_of_the_sdd():
    data = table.pack_table([ROW], NAME_SIZE)
    assert data[:3] == bytes([1, 0, 1])
    assert table.ROW_HEAD.unpack_from(data, 3) == (2, 0, 1800000000, 5, 0, 1, 1)
    assert data[3 + table.ROW_HEAD.size:].startswith(b"/seq/a.bin\0")


@pytest.mark.parametrize("field, value", [(7, struct.pack(">I", 1000000)), (13, b"\x02"), (11, b"\x03"),
                                          (12, b"\x03"), (14, b"\xff"), (14, b"\0")])
def test_read_rejects_what_the_component_rejects(field, value):
    data = bytearray(table.pack_table([ROW], NAME_SIZE))
    data[3 + field: 3 + field + len(value)] = value
    with pytest.raises(table.TableError):
        table.unpack_table(resealed(bytes(data)), NAME_SIZE)


def test_read_rejects_a_damaged_file():
    data = table.pack_table([ROW], NAME_SIZE)
    for bad in (data[:-1], bytes([2]) + data[1:], data[:20] + bytes([data[20] ^ 1]) + data[21:], b""):
        with pytest.raises(table.TableError):
            table.unpack_table(bad, NAME_SIZE)


@pytest.mark.parametrize("row", [dict(ROW, useconds=1000000), dict(ROW, mode="NEVER"), dict(ROW, fileName=""),
                                 dict(ROW, fileName="x" * 241), dict(ROW, extra=1), dict(ROW, enabled=1)])
def test_write_rejects_an_invalid_row(row):
    with pytest.raises(table.TableError):
        table.pack_table([row], NAME_SIZE)
