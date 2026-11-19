#!/usr/bin/env python3
"""Write or read a Svc::SeqScheduler table file (SDD section 4.2).

  seq_schedule_table.py rows.json schedule.tbl     write a table file from a JSON list of rows
  seq_schedule_table.py --read schedule.tbl        print the rows of a table file as JSON

Each row is a JSON object:

  {"timeBase": "TB_WORKSTATION_TIME", "seconds": 1800000000, "useconds": 0,
   "mode": "ABSOLUTE", "busyPolicy": "WAIT_FOR_RUNNING", "enabled": true,
   "fileName": "/seq/downlink.bin"}

"timeBase" and "useconds" default to TB_NONE and 0, "busyPolicy" to WAIT_FOR_RUNNING and
"enabled" to true when the row has a file name. An empty object is an empty row. The sizes are those of the default
F Prime configuration; pass --file-name-size when FileNameStringSize differs.
"""
import argparse
import json
import struct
import sys
import zlib

VERSION = 1
TIME_BASES = {"TB_NONE": 0, "TB_PROC_TIME": 1, "TB_WORKSTATION_TIME": 2, "TB_SC_TIME": 3, "TB_DONT_CARE": 0xFFFF}
MODES = {"ABSOLUTE": 0, "START_RELATIVE": 1, "TIME_BASE_RELATIVE": 2}
POLICIES = {"WAIT_FOR_RUNNING": 0, "CANCEL_RUNNING": 1, "SKIP_WHEN_RUNNING": 2}
ROW_HEAD = struct.Struct(">HBIIBBB")  # time base, time context, seconds, microseconds, mode, policy, enabled
HEADER = struct.Struct(">BH")
DIGEST = struct.Struct(">I")


class TableError(Exception):
    """A row or file that is not a valid table"""


def _code(names, value, what, row):
    """Numeric value of an enumeration given by name or number"""
    if isinstance(value, str) and value in names:
        return names[value]
    if isinstance(value, int) and not isinstance(value, bool) and value in names.values():
        return value
    raise TableError(f"row {row}: {what} {value!r} is not one of {', '.join(names)}")


def _name(names, value):
    """Name of an enumeration value"""
    return next(name for name, code in names.items() if code == value)


def pack_row(row, index, name_size):
    """One row of the file from its JSON object"""
    if not isinstance(row, dict):
        raise TableError(f"row {index}: not a JSON object")
    known = {"timeBase", "seconds", "useconds", "mode", "busyPolicy", "enabled", "fileName"}
    unknown = set(row) - known
    if unknown:
        raise TableError(f"row {index}: unknown field(s) {', '.join(sorted(unknown))}")
    file_name = row.get("fileName", "")
    enabled = row.get("enabled", bool(file_name))
    seconds = row.get("seconds", 0)
    useconds = row.get("useconds", 0)
    if not isinstance(file_name, str) or not isinstance(enabled, bool):
        raise TableError(f"row {index}: fileName must be a string and enabled a boolean")
    encoded = file_name.encode("ascii", errors="strict") if file_name.isascii() else None
    if encoded is None or b"\0" in encoded or len(encoded) > name_size:
        raise TableError(f"row {index}: fileName must be ASCII of at most {name_size} characters")
    if enabled and not encoded:
        raise TableError(f"row {index}: an enabled row needs a fileName")
    for what, value, limit in (("seconds", seconds, 0xFFFFFFFF), ("useconds", useconds, 999999)):
        if not isinstance(value, int) or isinstance(value, bool) or not 0 <= value <= limit:
            raise TableError(f"row {index}: {what} must be an integer from 0 to {limit}")
    head = ROW_HEAD.pack(
        _code(TIME_BASES, row.get("timeBase", "TB_NONE"), "timeBase", index),
        0,
        seconds,
        useconds,
        _code(MODES, row.get("mode", "ABSOLUTE"), "mode", index),
        _code(POLICIES, row.get("busyPolicy", "WAIT_FOR_RUNNING"), "busyPolicy", index),
        1 if enabled else 0,
    )
    return head + encoded.ljust(name_size + 1, b"\0")


def pack_table(rows, name_size):
    """The bytes of a table file"""
    if not isinstance(rows, list):
        raise TableError("the input must be a JSON list of rows")
    if len(rows) > 0xFFFF:
        raise TableError("a table file holds at most 65535 rows")
    body = HEADER.pack(VERSION, len(rows)) + b"".join(pack_row(row, i, name_size) for i, row in enumerate(rows))
    return body + DIGEST.pack(zlib.crc32(body) & 0xFFFFFFFF)


def unpack_table(data, name_size):
    """The rows of a table file"""
    row_size = ROW_HEAD.size + name_size + 1
    if len(data) < HEADER.size + DIGEST.size:
        raise TableError("file too short")
    version, count = HEADER.unpack_from(data)
    if version != VERSION:
        raise TableError(f"version {version} is not {VERSION}")
    if len(data) != HEADER.size + count * row_size + DIGEST.size:
        raise TableError(f"size {len(data)} does not match {count} rows of {row_size} bytes")
    if DIGEST.unpack_from(data, len(data) - DIGEST.size)[0] != zlib.crc32(data[: -DIGEST.size]) & 0xFFFFFFFF:
        raise TableError("checksum mismatch")
    rows = []
    for index in range(count):
        offset = HEADER.size + index * row_size
        base, _, seconds, useconds, mode, policy, enabled = ROW_HEAD.unpack_from(data, offset)
        try:
            name = data[offset + ROW_HEAD.size : offset + row_size].split(b"\0", 1)[0].decode("ascii")
        except UnicodeDecodeError:
            raise TableError(f"row {index}: file name is not ASCII") from None
        # The checks the component makes on a row (SDD section 4.2)
        if useconds > 999999:
            raise TableError(f"row {index}: microseconds {useconds} above 999999")
        if enabled > 1:
            raise TableError(f"row {index}: enabled is {enabled}, not 0 or 1")
        if enabled == 1 and not name:
            raise TableError(f"row {index}: enabled with no file name")
        try:
            rows.append(
                {
                    "timeBase": _name(TIME_BASES, base),
                    "seconds": seconds,
                    "useconds": useconds,
                    "mode": _name(MODES, mode),
                    "busyPolicy": _name(POLICIES, policy),
                    "enabled": enabled == 1,
                    "fileName": name,
                }
            )
        except StopIteration:
            raise TableError(f"row {index}: enumeration value out of range") from None
    return rows


def main(argv=None):
    """Command line"""
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input", help="JSON list of rows, or the table file with --read")
    parser.add_argument("output", nargs="?", help="table file to write")
    parser.add_argument("--read", action="store_true", help="print the rows of a table file as JSON")
    parser.add_argument("--file-name-size", type=int, default=240, help="FileNameStringSize (default 240)")
    args = parser.parse_args(argv)
    try:
        if args.read:
            with open(args.input, "rb") as handle:
                rows = unpack_table(handle.read(), args.file_name_size)
            json.dump(rows, sys.stdout, indent=2)
            sys.stdout.write("\n")
            return 0
        if args.output is None:
            parser.error("an output file is required")
        with open(args.input, encoding="utf-8") as handle:
            data = pack_table(json.load(handle), args.file_name_size)
        with open(args.output, "wb") as handle:
            handle.write(data)
        return 0
    except (TableError, OSError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
