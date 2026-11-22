"""Sample server resource usage using only the standard library.

Linux/WSL is the primary target because the meeting server runs there.  A
small PowerShell fallback is included so the same report format can be used
when the server is a native Windows process.
"""

from __future__ import annotations

import argparse
import csv
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import platform
import subprocess
import time


def _proc_status(pid: int) -> dict[str, float | int | str]:
    status_path = Path(f"/proc/{pid}/status")
    values: dict[str, float | int | str] = {}
    for line in status_path.read_text(encoding="utf-8").splitlines():
        key, _, raw = line.partition(":")
        raw = raw.strip()
        if key in {"Name", "State"}:
            values[key] = raw
        elif key in {"VmRSS", "VmSize"}:
            values[key] = int(raw.split()[0]) if raw else 0  # kB
        elif key == "Threads":
            values[key] = int(raw)
    return values


def _proc_ticks(pid: int) -> int:
    line = Path(f"/proc/{pid}/stat").read_text(encoding="utf-8")
    rest = line.rsplit(")", 1)[1].split()
    # rest[11] and rest[12] are utime and stime (fields 14 and 15).
    return int(rest[11]) + int(rest[12])


def _system_ticks() -> int:
    fields = Path("/proc/stat").read_text(encoding="utf-8").splitlines()[0].split()
    return sum(int(value) for value in fields[1:])


def _established_tcp_inodes() -> set[str]:
    inodes: set[str] = set()
    for path in (Path("/proc/net/tcp"), Path("/proc/net/tcp6")):
        if not path.exists():
            continue
        for line in path.read_text(encoding="utf-8").splitlines()[1:]:
            fields = line.split()
            # /proc/net/tcp state 01 is ESTABLISHED; do not count the
            # listening socket as an active client connection.
            if len(fields) >= 10 and fields[3] == "01":
                inodes.add(fields[9])
    return inodes


def _proc_tcp_count(pid: int) -> int:
    owned: set[str] = set()
    for link in Path(f"/proc/{pid}/fd").glob("*"):
        try:
            target = os.readlink(link)
        except OSError:
            continue
        if target.startswith("socket:[") and target.endswith("]"):
            owned.add(target[8:-1])
    return len(owned & _established_tcp_inodes())


class Sampler:
    def __init__(self, pid: int):
        self.pid = pid
        self.previous_time = time.monotonic()
        self.previous_process_ticks: int | None = None
        self.previous_system_ticks: int | None = None
        self.clock_ticks = os.sysconf("SC_CLK_TCK") if os.name != "nt" else 100

    def linux_sample(self) -> dict:
        status = _proc_status(self.pid)
        process_ticks = _proc_ticks(self.pid)
        system_ticks = _system_ticks()
        now = time.monotonic()
        cpu_percent = 0.0
        if self.previous_process_ticks is not None and self.previous_system_ticks is not None:
            process_delta = process_ticks - self.previous_process_ticks
            system_delta = system_ticks - self.previous_system_ticks
            if system_delta > 0:
                cpu_percent = process_delta / system_delta * (os.cpu_count() or 1) * 100
        self.previous_process_ticks = process_ticks
        self.previous_system_ticks = system_ticks
        self.previous_time = now
        return {
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "pid": self.pid,
            "alive": True,
            "cpu_percent": round(cpu_percent, 3),
            "rss_kb": int(status.get("VmRSS", 0)),
            "vms_kb": int(status.get("VmSize", 0)),
            "threads": int(status.get("Threads", 0)),
            "tcp_connections": _proc_tcp_count(self.pid),
        }

    def windows_sample(self) -> dict:
        command = (
            f"$p=Get-Process -Id {self.pid} -ErrorAction Stop; "
            "$tcp=(Get-NetTCPConnection -OwningProcess $p.Id -ErrorAction SilentlyContinue | Measure-Object).Count; "
            "[pscustomobject]@{cpu=$p.CPU;rss=$p.WorkingSet64;threads=$p.Threads.Count;tcp=$tcp} | ConvertTo-Json -Compress"
        )
        raw = subprocess.check_output(["powershell", "-NoProfile", "-Command", command], text=True)
        values = json.loads(raw)
        now = time.monotonic()
        cpu_seconds = float(values.get("cpu") or 0)
        cpu_percent = 0.0
        if self.previous_process_ticks is not None:
            elapsed = max(0.001, now - self.previous_time)
            cpu_percent = (cpu_seconds - self.previous_process_ticks) / elapsed / (os.cpu_count() or 1) * 100
        self.previous_process_ticks = cpu_seconds
        self.previous_time = now
        return {
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "pid": self.pid,
            "alive": True,
            "cpu_percent": round(max(0.0, cpu_percent), 3),
            "rss_kb": int(values.get("rss", 0)) // 1024,
            "vms_kb": 0,
            "threads": int(values.get("threads", 0)),
            "tcp_connections": int(values.get("tcp", 0)),
        }

    def sample(self) -> dict:
        if os.path.isdir(f"/proc/{self.pid}"):
            return self.linux_sample()
        if os.name == "nt":
            return self.windows_sample()
        raise ProcessLookupError(f"process {self.pid} is not available")


def _summary(samples: list[dict], process_exited: bool) -> dict:
    def peak(key: str) -> float | int:
        return max((sample[key] for sample in samples), default=0)

    cpu = [float(sample["cpu_percent"]) for sample in samples]
    return {
        "samples": len(samples),
        "process_exited": process_exited,
        "avg_cpu_percent": round(sum(cpu) / len(cpu), 3) if cpu else 0.0,
        "peak_cpu_percent": peak("cpu_percent"),
        "peak_rss_kb": peak("rss_kb"),
        "peak_threads": peak("threads"),
        "peak_tcp_connections": peak("tcp_connections"),
    }


def write_csv(path: Path, samples: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = ["timestamp", "pid", "alive", "cpu_percent", "rss_kb", "vms_kb", "threads", "tcp_connections"]
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(samples)


def monitor(pid: int, duration: float, interval: float) -> tuple[list[dict], bool]:
    sampler = Sampler(pid)
    samples: list[dict] = []
    deadline = time.monotonic() + duration
    process_exited = False
    while time.monotonic() < deadline or not samples:
        try:
            samples.append(sampler.sample())
        except (FileNotFoundError, ProcessLookupError, subprocess.CalledProcessError):
            process_exited = True
            break
        remaining = deadline - time.monotonic()
        if remaining > 0:
            time.sleep(min(interval, remaining))
    return samples, process_exited


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--duration", type=float, default=60)
    parser.add_argument("--interval", type=float, default=1)
    parser.add_argument("--output-json", default="qa/reports/resource-monitor.json")
    parser.add_argument("--output-csv", default="qa/reports/resource-monitor.csv")
    args = parser.parse_args()
    if args.duration <= 0 or args.interval <= 0:
        parser.error("--duration and --interval must be positive")

    samples, process_exited = monitor(args.pid, args.duration, args.interval)
    document = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "platform": platform.platform(),
        "pid": args.pid,
        "duration_s": args.duration,
        "interval_s": args.interval,
        "summary": _summary(samples, process_exited),
        "samples": samples,
    }
    json_path = Path(args.output_json)
    json_path.parent.mkdir(parents=True, exist_ok=True)
    json_path.write_text(json.dumps(document, ensure_ascii=False, indent=2), encoding="utf-8")
    write_csv(Path(args.output_csv), samples)
    print(json.dumps(document, ensure_ascii=False, indent=2))
    return 0 if samples and not process_exited else 1


if __name__ == "__main__":
    raise SystemExit(main())
