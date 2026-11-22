import uuid

import pytest

from protocol_client import ProtocolClient
from protocol_client.packets import LOGIN_SUCCESS


pytestmark = pytest.mark.integration


def _new_account() -> tuple[str, str, str]:
    token = uuid.uuid4().hex[:12]
    return f"15{token[:9]}", "qa_password", f"room_{token}"


def _register_and_login() -> tuple[ProtocolClient, int]:
    tel, password, name = _new_account()
    client = ProtocolClient().connect()
    assert client.register(tel, password, name) == 1
    result, user_id, returned_name = client.login(tel, password)
    assert result == LOGIN_SUCCESS
    assert returned_name == name
    return client, user_id


def test_create_and_join_room():
    owner, owner_id = _register_and_login()
    guest, guest_id = _register_and_login()

    try:
        create_result, room_id = owner.create_room(owner_id)
        assert create_result == 1
        assert room_id > 0

        join_result, joined_room_id = guest.join_room(guest_id, room_id)
        assert join_result == 1
        assert joined_room_id == room_id
    finally:
        owner.close()
        guest.close()


def test_join_missing_room_is_rejected():
    client, user_id = _register_and_login()
    try:
        result, returned_room_id = client.join_room(user_id, 2147483000)
        assert result == 0
        assert returned_room_id == 0
    finally:
        client.close()
