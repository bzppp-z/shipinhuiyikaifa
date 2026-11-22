import hashlib
import struct


PACK_BASE = 10000
PACK_REGISTER_RQ = PACK_BASE + 0
PACK_REGISTER_RS = PACK_BASE + 1
PACK_LOGIN_RQ = PACK_BASE + 2
PACK_LOGIN_RS = PACK_BASE + 3
PACK_CREATE_ROOM_RQ = PACK_BASE + 4
PACK_CREATE_ROOM_RS = PACK_BASE + 5
PACK_JOIN_ROOM_RQ = PACK_BASE + 6
PACK_JOIN_ROOM_RS = PACK_BASE + 7
PACK_ROOM_MEMBER = PACK_BASE + 8
PACK_AUDIO_FRAME = PACK_BASE + 9
PACK_VIDEO_FRAME = PACK_BASE + 10
PACK_LEAVE_ROOM_RQ = PACK_BASE + 11
PACK_AUDIO_REGISTER = PACK_BASE + 14
PACK_VIDEO_REGISTER = PACK_BASE + 15

REGISTER_SUCCESS = 1
TEL_EXISTS = 0
USER_NOT_FOUND = 0
PASSWORD_ERROR = 1
LOGIN_SUCCESS = 2

FIELD_SIZE = 40


def _fixed_field(value: str) -> bytes:
    raw = value.encode("utf-8")
    if len(raw) >= FIELD_SIZE:
        raise ValueError("protocol field must fit in 39 bytes plus NUL")
    return raw + b"\0" * (FIELD_SIZE - len(raw))


def md5_password(password: str) -> str:
    return hashlib.md5(password.encode("utf-8")).hexdigest()


def register_request(tel: str, password: str, name: str) -> bytes:
    return struct.pack(
        "<i40s40s40s",
        PACK_REGISTER_RQ,
        _fixed_field(tel),
        _fixed_field(name),
        _fixed_field(md5_password(password)),
    )


def login_request(tel: str, password: str) -> bytes:
    return struct.pack(
        "<i40s40s",
        PACK_LOGIN_RQ,
        _fixed_field(tel),
        _fixed_field(md5_password(password)),
    )


def create_room_request(user_id: int) -> bytes:
    return struct.pack("<ii", PACK_CREATE_ROOM_RQ, user_id)


def join_room_request(user_id: int, room_id: int) -> bytes:
    return struct.pack("<iii", PACK_JOIN_ROOM_RQ, user_id, room_id)


def audio_register_request(user_id: int) -> bytes:
    return struct.pack("<ii", PACK_AUDIO_REGISTER, user_id)


def video_register_request(user_id: int) -> bytes:
    return struct.pack("<ii", PACK_VIDEO_REGISTER, user_id)


def media_frame(packet_type: int, user_id: int, room_id: int, body: bytes) -> bytes:
    # The current client protocol uses type, user, room and three time fields.
    return struct.pack("<iiiiii", packet_type, user_id, room_id, 0, 0, 0) + body


def result_response(payload: bytes, expected_type: int) -> int:
    if len(payload) < 8:
        raise ValueError("response is shorter than a result packet")
    packet_type, result = struct.unpack("<ii", payload[:8])
    if packet_type != expected_type:
        raise ValueError(f"unexpected packet type: {packet_type}")
    return result


def login_response(payload: bytes) -> tuple[int, int, str]:
    if len(payload) < 52:
        raise ValueError("login response is shorter than expected")
    packet_type, result, user_id, raw_name = struct.unpack("<iii40s", payload[:52])
    if packet_type != PACK_LOGIN_RS:
        raise ValueError(f"unexpected packet type: {packet_type}")
    name = raw_name.split(b"\0", 1)[0].decode("utf-8", errors="replace")
    return result, user_id, name


def room_response(payload: bytes, expected_type: int) -> tuple[int, int]:
    if len(payload) < 12:
        raise ValueError("room response is shorter than expected")
    packet_type, result, room_id = struct.unpack("<iii", payload[:12])
    if packet_type != expected_type:
        raise ValueError(f"unexpected packet type: {packet_type}")
    return result, room_id
