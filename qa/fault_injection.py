"""Small, repeatable protocol fault probes used by stage eight.

The probes deliberately use the public TCP framing instead of reaching into
server internals.  A probe is successful when the server accepts the input
without taking down the listener and a fresh connection can be established
afterwards.
"""

from __future__ import annotations

import os
import socket
import struct
import time
import uuid
from dataclasses import asdict, dataclass

from protocol_client import ProtocolClient
from protocol_client.framing import pack_frame
from protocol_client.packets import (
    LOGIN_SUCCESS,
    login_request,
    register_request,
    video_register_request,
)


MAX_FRAME_SIZE = 1024 * 1024
UNKNOWN_PACKET = 0x7FFF_FFFE


@dataclass
class ProbeResult:
    name: str
    ok: bool
    duration_ms: float
    detail: str = ""
    healthy_after: bool = False
    error: str = ""

    def as_dict(self) -> dict:
        return asdict(self)


def endpoint() -> tuple[str, int, float]:
    return (
        os.getenv("MEETING_SERVER_HOST", "127.0.0.1"),
        int(os.getenv("MEETING_SERVER_PORT", "8000")),
        float(os.getenv("MEETING_SOCKET_TIMEOUT", "3")),
    )


def healthy_connection() -> None:
    host, port, timeout = endpoint()
    with socket.create_connection((host, port), timeout=timeout):
        pass


def raw_send(payload: bytes, *, chunks: tuple[int, ...] = ()) -> None:
    host, port, timeout = endpoint()
    with socket.create_connection((host, port), timeout=timeout) as sock:
        frame = struct.pack("<i", len(payload)) + payload
        if not chunks:
            sock.sendall(frame)
            return
        offset = 0
        for size in chunks:
            if offset >= len(frame):
                break
            sock.sendall(frame[offset : offset + size])
            offset += size
            time.sleep(0.01)
        if offset < len(frame):
            sock.sendall(frame[offset:])


def _account() -> tuple[str, str, str]:
    token = uuid.uuid4().hex[:12]
    return f"18{token[:9]}", "qa_password", f"fault_{token}"


def _login() -> tuple[str, int]:
    tel, password, name = _account()
    with ProtocolClient() as client:
        if client.register(tel, password, name) != 1:
            raise RuntimeError("fault probe account registration failed")
    client = ProtocolClient().connect()
    result, user_id, _ = client.login(tel, password)
    client.close()
    if result != LOGIN_SUCCESS:
        raise RuntimeError(f"fault probe login failed: result={result}")
    return tel, user_id


def _probe(name: str, action) -> ProbeResult:
    started = time.perf_counter()
    result = ProbeResult(name=name, ok=False, duration_ms=0.0)
    try:
        action()
        # A malformed packet may legitimately make only its own connection go
        # away.  The listener must still accept a new connection.
        healthy_connection()
        result.ok = True
        result.healthy_after = True
        result.detail = "listener accepted a fresh connection"
    except Exception as exc:  # noqa: BLE001 - report every probe and continue
        result.error = f"{type(exc).__name__}: {exc}"
    finally:
        result.duration_ms = round((time.perf_counter() - started) * 1000, 2)
    return result


def negative_length() -> None:
    host, port, timeout = endpoint()
    with socket.create_connection((host, port), timeout=timeout) as sock:
        sock.sendall(struct.pack("<i", -1))
        time.sleep(0.05)


def oversized_length() -> None:
    host, port, timeout = endpoint()
    with socket.create_connection((host, port), timeout=timeout) as sock:
        sock.sendall(struct.pack("<i", MAX_FRAME_SIZE + 1))
        time.sleep(0.05)


def unknown_packet() -> None:
    raw_send(struct.pack("<ii", UNKNOWN_PACKET, 0))


def truncated_known_packet() -> None:
    # A known message type with no fields must be ignored before the handler
    # can reinterpret the buffer as a larger C++ struct.
    raw_send(struct.pack("<i", 10000))


def partial_frame() -> None:
    host, port, timeout = endpoint()
    with socket.create_connection((host, port), timeout=timeout) as sock:
        payload = register_request("18000000000", "qa_password", "partial")
        frame = pack_frame(payload)
        sock.sendall(frame[:2])
        time.sleep(0.03)
        # Closing in the middle of the length/payload is the fault condition.


def abrupt_control_close() -> None:
    host, port, timeout = endpoint()
    sock = socket.create_connection((host, port), timeout=timeout)
    sock.sendall(pack_frame(login_request("18000000000", "wrong_password")))
    sock.close()


def abrupt_media_close() -> None:
    _, user_id = _login()
    host, port, timeout = endpoint()
    sock = socket.create_connection((host, port), timeout=timeout)
    sock.sendall(pack_frame(video_register_request(user_id)))
    sock.close()


def duplicate_media_register() -> None:
    _, user_id = _login()
    host, port, timeout = endpoint()
    with socket.create_connection((host, port), timeout=timeout) as sock:
        payload = pack_frame(video_register_request(user_id))
        sock.sendall(payload)
        sock.sendall(payload)
        time.sleep(0.05)


def illegal_room_id() -> None:
    _, user_id = _login()
    client = ProtocolClient().connect()
    try:
        result, room_id = client.join_room(user_id, 2_147_483_000)
        if result != 0 or room_id != 0:
            raise RuntimeError(f"invalid room was accepted: result={result}, room={room_id}")
    finally:
        client.close()


PROBES = {
    "negative-length": negative_length,
    "oversized-length": oversized_length,
    "unknown-packet": unknown_packet,
    "truncated-known-packet": truncated_known_packet,
    "partial-frame": partial_frame,
    "abrupt-control-close": abrupt_control_close,
    "abrupt-media-close": abrupt_media_close,
    "duplicate-media-register": duplicate_media_register,
    "illegal-room-id": illegal_room_id,
}


def run(names: list[str]) -> list[ProbeResult]:
    return [_probe(name, PROBES[name]) for name in names]


def report(results: list[ProbeResult]) -> dict:
    return {
        "host": endpoint()[0],
        "port": endpoint()[1],
        "probes": [item.as_dict() for item in results],
        "passed": sum(item.ok for item in results),
        "failed": sum(not item.ok for item in results),
    }

