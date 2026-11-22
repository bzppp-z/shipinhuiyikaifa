import socket
import struct


MAX_FRAME_SIZE = 1024 * 1024


def pack_frame(payload: bytes) -> bytes:
    if not payload or len(payload) > MAX_FRAME_SIZE:
        raise ValueError("payload length is outside the allowed range")
    return struct.pack("<i", len(payload)) + payload


def recv_exact(sock: socket.socket, size: int) -> bytes:
    chunks = bytearray()
    while len(chunks) < size:
        chunk = sock.recv(size - len(chunks))
        if not chunk:
            raise ConnectionError("peer closed before the frame was complete")
        chunks.extend(chunk)
    return bytes(chunks)


def recv_frame(sock: socket.socket) -> bytes:
    raw_size = recv_exact(sock, 4)
    (size,) = struct.unpack("<i", raw_size)
    if size <= 0 or size > MAX_FRAME_SIZE:
        raise ValueError(f"invalid frame size: {size}")
    return recv_exact(sock, size)
