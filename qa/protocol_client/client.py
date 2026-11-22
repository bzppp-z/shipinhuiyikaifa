import os
import socket

from .framing import pack_frame, recv_frame
from .packets import (
    PACK_CREATE_ROOM_RS,
    PACK_JOIN_ROOM_RS,
    LOGIN_SUCCESS,
    PACK_LOGIN_RS,
    PACK_REGISTER_RS,
    create_room_request,
    audio_register_request,
    join_room_request,
    video_register_request,
    login_request,
    login_response,
    register_request,
    result_response,
    room_response,
)


class ProtocolClient:
    def __init__(self, host: str | None = None, port: int | None = None):
        self.host = host or os.getenv("MEETING_SERVER_HOST", "127.0.0.1")
        self.port = port or int(os.getenv("MEETING_SERVER_PORT", "8000"))
        self.timeout = float(os.getenv("MEETING_SOCKET_TIMEOUT", "3"))
        self.sock: socket.socket | None = None

    def connect(self) -> "ProtocolClient":
        self.sock = socket.create_connection((self.host, self.port), self.timeout)
        self.sock.settimeout(self.timeout)
        return self

    def close(self) -> None:
        if self.sock is not None:
            self.sock.close()
            self.sock = None

    def __enter__(self) -> "ProtocolClient":
        return self.connect()

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def request(self, payload: bytes) -> bytes:
        self.send_only(payload)
        return self.receive()

    def send_only(self, payload: bytes) -> None:
        if self.sock is None:
            raise RuntimeError("client is not connected")
        self.sock.sendall(pack_frame(payload))

    def receive(self, timeout: float | None = None) -> bytes:
        if self.sock is None:
            raise RuntimeError("client is not connected")
        previous_timeout = self.sock.gettimeout()
        if timeout is not None:
            self.sock.settimeout(timeout)
        try:
            return recv_frame(self.sock)
        finally:
            if timeout is not None:
                self.sock.settimeout(previous_timeout)

    def register(self, tel: str, password: str, name: str) -> int:
        payload = self.request(register_request(tel, password, name))
        return result_response(payload, PACK_REGISTER_RS)

    def login(self, tel: str, password: str) -> tuple[int, int, str]:
        payload = self.request(login_request(tel, password))
        result, user_id, name = login_response(payload)
        return result, user_id, name

    def create_room(self, user_id: int) -> tuple[int, int]:
        return room_response(self.request(create_room_request(user_id)), PACK_CREATE_ROOM_RS)

    def join_room(self, user_id: int, room_id: int) -> tuple[int, int]:
        return room_response(self.request(join_room_request(user_id, room_id)), PACK_JOIN_ROOM_RS)

    def register_audio(self, user_id: int) -> None:
        self.send_only(audio_register_request(user_id))

    def register_video(self, user_id: int) -> None:
        self.send_only(video_register_request(user_id))


__all__ = ["LOGIN_SUCCESS", "ProtocolClient"]
