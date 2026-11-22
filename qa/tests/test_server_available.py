import os
import socket

import pytest


pytestmark = pytest.mark.integration


def test_server_port_open():
    """The regression runner can reach the running Linux meeting server."""
    host = os.getenv("MEETING_SERVER_HOST", "127.0.0.1")
    port = int(os.getenv("MEETING_SERVER_PORT", "8000"))
    timeout = float(os.getenv("MEETING_CONNECT_TIMEOUT", "3"))

    with socket.create_connection((host, port), timeout=timeout):
        pass
