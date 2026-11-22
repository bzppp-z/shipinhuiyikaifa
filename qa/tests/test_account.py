import uuid

import pytest

from protocol_client import ProtocolClient
from protocol_client.packets import LOGIN_SUCCESS, REGISTER_SUCCESS, TEL_EXISTS, USER_NOT_FOUND


pytestmark = pytest.mark.integration


def _new_account() -> tuple[str, str, str]:
    token = uuid.uuid4().hex[:12]
    return f"13{token[:9]}", "qa_password", f"qa_{token}"


def test_register_and_login():
    tel, password, name = _new_account()

    with ProtocolClient() as client:
        assert client.register(tel, password, name) == REGISTER_SUCCESS

    with ProtocolClient() as client:
        result, user_id, returned_name = client.login(tel, password)

    assert result == LOGIN_SUCCESS
    assert user_id > 0
    assert returned_name == name


def test_duplicate_phone_is_rejected():
    tel, password, name = _new_account()

    with ProtocolClient() as client:
        assert client.register(tel, password, name) == REGISTER_SUCCESS

    with ProtocolClient() as client:
        assert client.register(tel, password, f"other_{name}") == TEL_EXISTS


def test_unknown_phone_cannot_login():
    tel, password, _ = _new_account()

    with ProtocolClient() as client:
        result, user_id, name = client.login(tel, password)

    assert result == USER_NOT_FOUND
    assert user_id == 0
    assert name == ""
