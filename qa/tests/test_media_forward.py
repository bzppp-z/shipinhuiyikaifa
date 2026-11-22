import socket
import time
import uuid

import pytest

from protocol_client import ProtocolClient
from protocol_client.packets import (
    LOGIN_SUCCESS,
    PACK_AUDIO_FRAME,
    PACK_VIDEO_FRAME,
    media_frame,
)


pytestmark = pytest.mark.media


def _new_account() -> tuple[str, str, str]:
    token = uuid.uuid4().hex[:12]
    return f"16{token[:9]}", "qa_password", f"media_{token}"


def _register_and_login() -> tuple[ProtocolClient, int]:
    tel, password, name = _new_account()
    client = ProtocolClient().connect()
    assert client.register(tel, password, name) == 1
    result, user_id, returned_name = client.login(tel, password)
    assert result == LOGIN_SUCCESS
    assert returned_name == name
    return client, user_id


@pytest.mark.parametrize(
    ("register_method", "packet_type", "body"),
    [
        ("register_audio", PACK_AUDIO_FRAME, b"qa-audio-frame"),
        ("register_video", PACK_VIDEO_FRAME, b"qa-video-frame"),
    ],
)
def test_media_frame_is_forwarded_to_other_room_member(register_method, packet_type, body):
    owner_control, owner_id = _register_and_login()
    guest_control, guest_id = _register_and_login()
    owner_media = ProtocolClient().connect()
    guest_media = ProtocolClient().connect()

    try:
        result, room_id = owner_control.create_room(owner_id)
        assert result == 1
        assert guest_control.join_room(guest_id, room_id)[0] == 1

        getattr(owner_media, register_method)(owner_id)
        getattr(guest_media, register_method)(guest_id)
        time.sleep(0.1)

        payload = media_frame(packet_type, owner_id, room_id, body)
        owner_media.send_only(payload)
        assert guest_media.receive(timeout=2.0) == payload

        with pytest.raises(socket.timeout):
            owner_media.receive(timeout=0.2)
    finally:
        owner_media.close()
        guest_media.close()
        owner_control.close()
        guest_control.close()
