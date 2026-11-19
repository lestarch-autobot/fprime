#!/usr/bin/env python3
"""Bench actions of the SeqScheduler system tests on a native Linux Ref.

Every action is one command line of bench.json, run from the campaign root. The bench has no
hardware: "flash" installs the Ref binary into the work directory, "dump" reads it back,
"gds" starts the ground system and then Ref, "reset" stops and starts the Ref process.
Ref runs with the work directory as its working directory, so table, sequence, parameter and
data product files are all found there.
"""
import argparse
import hashlib
import json
import os
import re
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
START_FILE = WORK / "ref.start"
RAW_TIME = re.compile(r",\((\d+)\((\d+)\)-(\d+):(\d+)\),([^,]+),\d+,(.*)$")
YEAR_2100 = 4102444800  # 2100-01-01T00:00:00Z


def start_ref(port):
    out = open(WORK / "ref.out", "a")
    proc = subprocess.Popen(["./Ref", "-a", "127.0.0.1", "-p", str(port)], cwd=WORK, stdout=out,
                            stderr=subprocess.STDOUT, start_new_session=True)
    PID_FILE.write_text(f"{proc.pid}\n")
    START_FILE.write_text(f"{time.time():.3f}\n")
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
    for table in ("T-BASE", "T-BASE-B"):
        write_table(argparse.Namespace(table=table))
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


def recorder(args):
    """The event recorder, with the ground receive time of each event.

    Runs `fprime-cli events`, which logs every event, as it is received, as one line of event.log under `logs`.
    Each line of that log is copied to `logs`/event-recv.log behind the host time at which it appeared there
    (polled every 20 ms). What `fprime-cli events` prints is not used: it is not one line per event."""
    before = set(Path(args.logs).glob("fprime-cli-*/event.log"))
    cmd = ["fprime-cli", "events", "--dictionary", args.dictionary, "--logs", args.logs]
    cli = subprocess.Popen(cmd, stdout=subprocess.DEVNULL)
    stopping = []

    def stop(*_):
        stopping.append(True)
        cli.terminate()

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    log, held = None, ""
    with open(Path(args.logs) / "event-recv.log", "w", buffering=1) as out:
        while True:
            ending = bool(stopping) or cli.poll() is not None  # read once more after the recorder has ended
            if log is None:
                new = sorted(set(Path(args.logs).glob("fprime-cli-*/event.log")) - before)
                log = open(new[-1], encoding="utf-8", errors="replace") if new else None
            if log is not None:
                held += log.read()
                now = time.time()
                *whole, held = held.split("\n")
                for line in whole:
                    if line.strip():
                        out.write(f"{now:.6f} {line}\n")
            if ending:
                break
            time.sleep(0.02)
    cli.wait()


def reset(args):
    stop_ref()
    time.sleep(1.0)
    start_ref(args.port)


def evidence(data):
    """One evidence row of the run record (the test tool keeps stdout lines of this form)."""
    print("EVIDENCE " + json.dumps(data, sort_keys=True))


def last_row(logs, log_name, channel=None):
    """Raw time and age of the newest recorder row (of one channel, if named)."""
    files = sorted(Path(logs).glob(f"fprime-cli-*/{log_name}"))
    if not files:
        return None
    found = None
    for line in files[-1].read_text(errors="replace").splitlines():
        m = RAW_TIME.search(line)
        if m and (channel is None or m.group(5).endswith("." + channel)):
            found = (m, line)
    if found is None:
        return None
    m, line = found
    return {"base": int(m.group(1)), "sec": int(m.group(3)), "usec": int(m.group(4)),
            "age_s": round(time.time() - files[-1].stat().st_mtime, 3), "value": m.group(6)}


def fsw_now(args):
    """FSW time of the newest recorded event: the source of every 'FSW time + N s' of the plan."""
    evidence(last_row(args.logs, "event.log"))


def next_due(args):
    """Newest NextTimeDue sample of a scheduler and the FSW time it was written at."""
    evidence(last_row(args.logs, "channel.log", args.instance + ".NextTimeDue"))


def ref_start(args):
    """Ground time at which the bench started the running Ref process."""
    evidence({"start": float(START_FILE.read_text()), "pid": int(PID_FILE.read_text())})


def manifest_versions(args):
    """Describe strings of the repositories the run's manifest pins (`fth run` names the manifest in FTH_MANIFEST)."""
    path = os.environ.get("FTH_MANIFEST")
    if not path:
        evidence({"describe": []})
        return
    with open(path, encoding="utf-8") as manifest:
        repos = json.load(manifest).get("repos", [])
    evidence({"describe": sorted({str(r["describe"]) for r in repos if r.get("describe")})})


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
            "T-FIELDS": ("seq_sched.tbl", t_fields),
            "T-TRUNC": ("t_trunc.tbl", base_rows()), "T-BADCRC": ("t_badcrc.tbl", base_rows())}


# Table files written as T-BASE and then damaged: its last byte removed, or one row byte inverted
DAMAGED = {"T-TRUNC": "truncate", "T-BADCRC": "invert"}
HEADER_SIZE = 3  # version byte and big-endian U16 row count


def write_table(args):
    name, rows = tables()[args.table]
    source = WORK / (args.table + ".json")
    source.write_text(json.dumps(rows, indent=1) + "\n")
    subprocess.run([sys.executable, str(TABLE_SCRIPT), str(source), str(WORK / name)], check=True)
    damage = DAMAGED.get(args.table)
    if damage is not None:
        data = bytearray((WORK / name).read_bytes())
        if damage == "truncate":
            del data[-1]
        else:
            data[HEADER_SIZE] ^= 0xFF  # first byte of row 0
        (WORK / name).write_bytes(bytes(data))
    print(f"bench: wrote {args.table} to {name}")


def read_table(args):
    """Rows of a table file of the bench file system as the table script reads it (checksum verified)."""
    done = subprocess.run([sys.executable, str(TABLE_SCRIPT), "--read", str(WORK / args.name)],
                          capture_output=True, text=True, check=False)
    rows = json.loads(done.stdout) if done.returncode == 0 else None
    evidence({"name": args.name, "valid": done.returncode == 0, "rows": rows, "error": done.stderr.strip()})


def read_dp(args):
    """The newest data product file written by Ref, decoded with the dictionary by fprime-dp."""
    files = sorted((WORK / "DpCat").glob("*.fdp"), key=lambda f: f.stat().st_mtime)
    if not files:
        evidence(None)
        return
    out = WORK / "dp-decoded.json"
    done = subprocess.run(["fprime-dp", "decode", "-b", str(files[-1]), "-d", args.dictionary, "-o", str(out)],
                          capture_output=True, text=True, check=False)
    decoded = json.loads(out.read_text()) if done.returncode == 0 and out.is_file() else None
    rows = None
    if decoded is not None:
        rows = []
        for record in decoded["Records"]:
            row = {key: plain(value) for key, value in record["Data"].items() if key != "time"}
            for key in ("timeBase", "seconds", "useconds"):
                row[key] = plain(record["Data"]["time"][key])
            row["record"] = record["Record"]["record_name"]
            rows.append(row)
    evidence({"file": files[-1].name, "files": len(files), "container": plain(decoded["Header"]["Id"]) if decoded else None,
              "rows": rows, "error": done.stderr.strip()[-300:]})


def plain(field):
    """Value of a decoded data-product field."""
    return field["value"] if isinstance(field, dict) else field


def count_dp(args):
    """Number of data product files Ref has written so far."""
    evidence({"files": len(list((WORK / "DpCat").glob("*.fdp")))})


def hash_file(args):
    """Size and digest of a file of the bench file system."""
    data = (WORK / args.name).read_bytes() if (WORK / args.name).is_file() else b""
    evidence({"name": args.name, "size": len(data), "sha256": hashlib.sha256(data).hexdigest()})


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
    p = sub.add_parser("recorder")
    p.add_argument("dictionary")
    p.add_argument("logs")
    p.set_defaults(fn=recorder)
    p = sub.add_parser("reset")
    p.add_argument("--port", type=int, default=50000)
    p.set_defaults(fn=reset)
    p = sub.add_parser("write-table")
    p.add_argument("table", choices=sorted(tables()))
    p.set_defaults(fn=write_table)
    p = sub.add_parser("fsw-now")
    p.add_argument("logs")
    p.set_defaults(fn=fsw_now)
    p = sub.add_parser("next-due")
    p.add_argument("logs")
    p.add_argument("instance")
    p.set_defaults(fn=next_due)
    p = sub.add_parser("ref-start")
    p.set_defaults(fn=ref_start)
    p = sub.add_parser("manifest-versions")
    p.set_defaults(fn=manifest_versions)
    p = sub.add_parser("read-table")
    p.add_argument("name")
    p.set_defaults(fn=read_table)
    p = sub.add_parser("read-dp")
    p.add_argument("dictionary")
    p.set_defaults(fn=read_dp)
    p = sub.add_parser("count-dp")
    p.set_defaults(fn=count_dp)
    p = sub.add_parser("hash-file")
    p.add_argument("name")
    p.set_defaults(fn=hash_file)
    args = parser.parse_args()
    args.fn(args)


if __name__ == "__main__":
    main()
