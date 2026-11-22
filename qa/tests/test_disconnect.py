import uuid

import pytest

from protocol_client import ProtocolClient
from protocol_client.packets import LOGIN_SUCCESS


pytestmark = pytest.mark.integration


def test_control_disconnect_does_not_block_a_new_login():
    token = uuid.uuid4().hex[:12]
    tel = f"17{token[:9]}"
    password = "qa_password"
    name = f"disconnect_{token}"

    first = ProtocolClient().connect()
    assert first.register(tel, password, name) == 1
    assert first.login(tel, password)[0] == LOGIN_SUCCESS
    first.close()

    second = ProtocolClient().connect()
    try:
        assert second.login(tel, password)[0] == LOGIN_SUCCESS
    finally:
        second.close()
