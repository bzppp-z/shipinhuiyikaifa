"""Repeatable load scenarios for the meeting server.

The script intentionally uses the public TCP protocol and synthetic media frames.
It does not require camera or microphone hardware.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import socket
import struct
import sys
import threading
import time
import uuid
from concurrent.futures import ThreadPoolExecutor, as_completed

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "qa"))

from protocol_client import ProtocolClient  # noqa: E402
from protocol_client.framing import pack_frame, recv_frame  # noqa: E402
from protocol_client.packets import (  # noqa: E402
    LOGIN_SUCCESS,
    PACK_VIDEO_FRAME,
    media_frame,
)


def percentile(values: list[float], p: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    index = min(len(ordered) - 1, int((len(ordered) - 1) * p))
    return ordered[index]


def account(index: int) -> tuple[str, str, str]:
    token = uuid.uuid4().hex[:12]
    return f"20{index:02d}{token[:7]}", "load_password", f"load_{index}_{token}"


def register_login(index: int) -> dict:
    tel, password, name = account(index)
    started = time.perf_counter()
    try:
        with ProtocolClient() as client:
            if client.register(tel, password, name) != 1:
                return {"ok": False, "error": "register_failed"}
        with ProtocolClient() as client:
            result, user_id, returned_name = client.login(tel, password)
        return {
            "ok": result == LOGIN_SUCCESS and returned_name == name,
            "user_id": user_id,
            "latency_ms": (time.perf_counter() - started) * 1000,
            "error": "" if result == LOGIN_SUCCESS else f"login_result_{result}",
        }
    except Exception as exc:  # noqa: BLE001 - load reports must continue collecting
        return {"ok": False, "error": type(exc).__name__}


def login_pressure(users: int, workers: int) -> dict:
    started = time.perf_counter()
    with ThreadPoolExecutor(max_workers=workers) as pool:
        results = list(pool.map(register_login, range(users)))
    elapsed = time.perf_counter() - started
    latencies = [r["latency_ms"] for r in results if r.get("ok")]
    return {
        "scenario": "login",
        "users": users,
        "workers": workers,
        "duration_s": round(elapsed, 3),
        "success": sum(bool(r.get("ok")) for r in results),
        "failed": sum(not bool(r.get("ok")) for r in results),
        "throughput_per_s": round(users / elapsed, 2) if elapsed else 0,
        "p95_latency_ms": round(percentile(latencies, 0.95), 2),
        "errors": [r.get("error") for r in results if r.get("error")],
    }


def setup_room(room_count: int, users_per_room: int) -> tuple[list[ProtocolClient], list[int]]:
    total = room_count * users_per_room
    with ThreadPoolExecutor(max_workers=min(50, total)) as pool:
        setup_results = list(pool.map(_login_client, range(total)))
    clients = [result[0] for result in setup_results]
    user_ids = [result[1] for result in setup_results]

    def create_room(room_index: int) -> tuple[int, int]:
        owner_index = room_index * users_per_room
        result, room_id = clients[owner_index].create_room(user_ids[owner_index])
        if result != 1:
            raise RuntimeError("room creation failed")
        return room_index, room_id

    with ThreadPoolExecutor(max_workers=room_count) as pool:
        room_results = list(pool.map(create_room, range(room_count)))
    room_ids = dict(room_results)

    def join_room(index: int) -> bool:
        room_index = index // users_per_room
        if index % users_per_room == 0:
            return True
        result, joined = clients[index].join_room(user_ids[index], room_ids[room_index])
        return result == 1 and joined == room_ids[room_index]

    with ThreadPoolExecutor(max_workers=min(50, total)) as pool:
        if not all(pool.map(join_room, range(total))):
            raise RuntimeError("room join failed")
    return clients, user_ids


def _login_client(index: int) -> tuple[ProtocolClient, int]:
    tel, password, name = account(index)
    client = ProtocolClient().connect()
    if client.register(tel, password, name) != 1:
        client.close()
        raise RuntimeError("register failed during room setup")
    result, user_id, _ = client.login(tel, password)
    if result != LOGIN_SUCCESS:
        client.close()
        raise RuntimeError("login failed during room setup")
    return client, user_id


def room_pressure(room_count: int, users_per_room: int) -> dict:
    started = time.perf_counter()
    clients: list[ProtocolClient] = []
    try:
        clients, _ = setup_room(room_count, users_per_room)
        return {
            "scenario": "rooms",
            "rooms": room_count,
            "users_per_room": users_per_room,
            "users": room_count * users_per_room,
            "success": room_count * users_per_room,
            "failed": 0,
            "duration_s": round(time.perf_counter() - started, 3),
        }
    except Exception as exc:  # noqa: BLE001
        return {
            "scenario": "rooms",
            "rooms": room_count,
            "users_per_room": users_per_room,
            "users": room_count * users_per_room,
            "success": 0,
            "failed": room_count * users_per_room,
            "duration_s": round(time.perf_counter() - started, 3),
            "errors": [type(exc).__name__],
        }
    finally:
        for client in clients:
            client.close()


def _media_receiver(client: ProtocolClient, deadline: float, stats: dict, lock: threading.Lock) -> None:
    while time.monotonic() < deadline:
        try:
            payload = client.receive(timeout=min(0.5, max(0.05, deadline - time.monotonic())))
            if len(payload) < 24 + 16:
                continue
            body = payload[24:]
            _, sent_at = struct.unpack("<Qd", body[:16])
            with lock:
                stats["received"] += 1
                stats["latencies_ms"].append((time.monotonic() - sent_at) * 1000)
        except socket.timeout:
            continue
        except (ConnectionError, OSError):
            break


def media_pressure(users: int, duration: int, fps: int) -> dict:
    controls: list[ProtocolClient] = []
    media: list[ProtocolClient] = []
    user_ids: list[int] = []
    receivers: list[threading.Thread] = []
    stats = {"sent": 0, "received": 0, "latencies_ms": []}
    lock = threading.Lock()
    started = time.perf_counter()
    try:
        for index in range(users):
            client, user_id = _login_client(index + 3000)
            controls.append(client)
            user_ids.append(user_id)
            if index == 0:
                result, room_id = client.create_room(user_id)
                if result != 1:
                    raise RuntimeError("room creation failed")
            else:
                if client.join_room(user_id, room_id)[0] != 1:
                    raise RuntimeError("room join failed")
            media_client = ProtocolClient().connect()
            media_client.register_video(user_id)
            media.append(media_client)
        deadline = time.monotonic() + duration
        for client in media:
            thread = threading.Thread(target=_media_receiver, args=(client, deadline, stats, lock), daemon=True)
            thread.start()
            receivers.append(thread)
        interval = 1.0 / fps
        sequence = 0
        while time.monotonic() < deadline:
            for index, client in enumerate(media):
                body = struct.pack("<Qd", sequence, time.monotonic()) + bytes(256)
                client.send_only(media_frame(PACK_VIDEO_FRAME, user_ids[index], room_id, body))
                stats["sent"] += 1
                sequence += 1
            time.sleep(interval)
        for thread in receivers:
            thread.join(timeout=2)
        return {
            "scenario": "media",
            "users": users,
            "duration_s": round(time.perf_counter() - started, 3),
            "sent_frames": stats["sent"],
            "received_frames": stats["received"],
            "delivery_ratio": round(stats["received"] / max(1, stats["sent"] * (users - 1)), 4),
            "p95_latency_ms": round(percentile(stats["latencies_ms"], 0.95), 2),
        }
    finally:
        for client in media + controls:
            client.close()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--scenario", choices=("login", "rooms", "media", "all"), default="all")
    parser.add_argument("--users", type=int, default=5)
    parser.add_argument("--workers", type=int, default=20)
    parser.add_argument("--rooms", type=int, default=5)
    parser.add_argument("--users-per-room", type=int, default=10)
    parser.add_argument("--duration", type=int, default=60)
    parser.add_argument("--fps", type=int, default=2)
    parser.add_argument("--output", default="qa/reports/load-result.json")
    args = parser.parse_args()

    results = []
    if args.scenario in ("login", "all"):
        results.append(login_pressure(args.users, args.workers))
    if args.scenario in ("rooms", "all"):
        results.append(room_pressure(args.rooms, args.users_per_room))
    if args.scenario in ("media", "all"):
        results.append(media_pressure(args.users, args.duration, args.fps))
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    document = {"generated_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), "results": results}
    output.write_text(json.dumps(document, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(document, ensure_ascii=False, indent=2))
    return 0 if all(result.get("failed", 0) == 0 for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
