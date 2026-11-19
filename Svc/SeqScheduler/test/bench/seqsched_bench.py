#!/usr/bin/env python3
"""Bench actions of the SeqScheduler system tests on a native Linux Ref.

Every action is one command line of bench.json, run from the campaign root. The bench has no
hardware: "flash" installs the Ref binary into the work directory, "dump" reads it back,
"gds" starts the ground system and then Ref, "reset" stops and starts the Ref process.
Ref runs with the work directory as its working directory, so table, sequence, parameter and
data product files are all found there.
"""
import argparse
import os
import shutil
import signal
import socket
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
TABLE_SCRIPT = HERE.parents[1] / "scripts" / "seq_schedule_table.py"
FILES = HERE.parent / "int" / "files"
WORK = Path("bench-work")
PID_FILE = WORK / "ref.pid"
YEAR_2100 = 4102444800  # 2100-01-01T00:00:00Z


def start_ref(port):
    out = open(WORK / "ref.out", "a")
    proc = subprocess.Popen(["./Ref", "-a", "127.0.0.1", "-p", str(port)], cwd=WORK, stdout=out,
                            stderr=subprocess.STDOUT, start_new_session=True)
    PID_FILE.write_text(f"{proc.pid}\n")
    print(f"bench: Ref started pid={proc.pid}", flush=True)
    return proc


def stop_ref():
    if not PID_FILE.is_file():
        return
    pid = int(PID_FILE.read_text())
    PID_FILE.unlink()
    try:
        os.kill(pid, signal.SIGTERM)
    except ProcessLookupError:
        return
    for _ in range(100):
        try:
            os.kill(pid, 0)
        except ProcessLookupError:
            return
        time.sleep(0.1)
    os.kill(pid, signal.SIGKILL)


def flash(args):
    stop_ref()
    shutil.rmtree(WORK, ignore_errors=True)
    WORK.mkdir(parents=True)
    shutil.copyfile(args.image, WORK / "Ref")
    os.chmod(WORK / "Ref", 0o755)
    for name in os.listdir(FILES):
        shutil.copyfile(FILES / name, WORK / name)
    # No debugger on a native bench: the host name stands where the tool expects a probe serial
    print(f"Serial# = {socket.gethostname()}")


def dump(args):
    shutil.copyfile(WORK / "Ref", args.readback)


def gds(args):
    """Ground system first, then Ref; stays in the foreground until terminated."""
    cmd = ["fprime-gds", "-n", "--gui", "none", "--dictionary", args.dictionary, "--logs", args.logs,
           "--ip-port", str(args.port)]
    ground = subprocess.Popen(cmd)

    def stop(*_):
        stop_ref()
        ground.terminate()
        sys.exit(0)

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    time.sleep(args.delay)
    start_ref(args.port)
    ground.wait()
    stop_ref()


def reset(args):
    stop_ref()
    time.sleep(1.0)
    start_ref(args.port)


def base_rows(file_name="short.bin"):
    return [{"timeBase": "TB_WORKSTATION_TIME", "seconds": YEAR_2100, "mode": "ABSOLUTE", "fileName": file_name,
             "busyPolicy": "WAIT_FOR_RUNNING", "enabled": False} for _ in range(16)]


def tables():
    t_base = base_rows()
    t_base_b = base_rows()
    t_base_b[0]["fileName"] = "short2.bin"
    t_fields = base_rows()
    t_fields[14] = {"timeBase": "TB_SC_TIME", "seconds": YEAR_2100, "mode": "ABSOLUTE", "fileName": "short2.bin",
                    "busyPolicy": "CANCEL_RUNNING", "enabled": True}
    t_fields[15] = {"timeBase": "TB_NONE", "seconds": 1000000, "mode": "START_RELATIVE", "fileName": "short.bin",
                    "busyPolicy": "SKIP_WHEN_RUNNING", "enabled": False}
    return {"T-BASE": ("seq_sched.tbl", t_base), "T-BASE-B": ("seq_sched_b.tbl", t_base_b),
            "T-FIELDS": ("seq_sched.tbl", t_fields)}


def write_table(args):
    import json
    name, rows = tables()[args.table]
    source = WORK / (args.table + ".json")
    source.write_text(json.dumps(rows, indent=1) + "\n")
    subprocess.run([sys.executable, str(TABLE_SCRIPT), str(source), str(WORK / name)], check=True)
    print(f"bench: wrote {args.table} to {name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    p = sub.add_parser("flash")
    p.add_argument("image")
    p.set_defaults(fn=flash)
    p = sub.add_parser("dump")
    p.add_argument("readback")
    p.set_defaults(fn=dump)
    p = sub.add_parser("gds")
    p.add_argument("dictionary")
    p.add_argument("logs")
    p.add_argument("--port", type=int, default=50000)
    p.add_argument("--delay", type=float, default=6.0)
    p.set_defaults(fn=gds)
    p = sub.add_parser("reset")
    p.add_argument("--port", type=int, default=50000)
    p.set_defaults(fn=reset)
    p = sub.add_parser("write-table")
    p.add_argument("table", choices=sorted(tables()))
    p.set_defaults(fn=write_table)
    args = parser.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
