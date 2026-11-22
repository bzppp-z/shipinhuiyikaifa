from concurrent.futures import ThreadPoolExecutor
import uuid

import pytest

from protocol_client import ProtocolClient
from protocol_client.packets import LOGIN_SUCCESS


pytestmark = pytest.mark.integration


def _register_and_login(index: int) -> bool:
    token = uuid.uuid4().hex[:12]
    tel = f"18{index}{token[:8]}"
    password = "qa_password"
    name = f"concurrent_{index}_{token}"
    with ProtocolClient() as client:
        if client.register(tel, password, name) != 1:
            return False
    with ProtocolClient() as client:
        return client.login(tel, password)[0] == LOGIN_SUCCESS


def test_multiple_users_can_register_and_login_concurrently():
    with ThreadPoolExecutor(max_workers=5) as pool:
        results = list(pool.map(_register_and_login, range(5)))
    assert results == [True] * 5
