import socket
import struct
import uuid

import pytest

from protocol_client.framing import pack_frame, recv_frame
from protocol_client.packets import register_request, PACK_REGISTER_RS, result_response


pytestmark = pytest.mark.smoke


def test_frame_round_trip():
    sender, receiver = socket.socketpair()
    try:
        sender.sendall(pack_frame(b"protocol-payload"))
        assert recv_frame(receiver) == b"protocol-payload"
    finally:
        sender.close()
        receiver.close()


def test_frame_can_be_received_in_parts():
    sender, receiver = socket.socketpair()
    try:
        frame = pack_frame(b"split-payload")
        sender.sendall(frame[:2])
        sender.sendall(frame[2:7])
        sender.sendall(frame[7:])
        assert recv_frame(receiver) == b"split-payload"
    finally:
        sender.close()
        receiver.close()


def test_invalid_frame_length_is_rejected():
    sender, receiver = socket.socketpair()
    try:
        sender.sendall(struct.pack("<i", -1))
        with pytest.raises(ValueError):
            recv_frame(receiver)
    finally:
        sender.close()
        receiver.close()


@pytest.mark.integration
def test_server_accepts_split_length_prefix_and_payload():
    from protocol_client import ProtocolClient

    token = uuid.uuid4().hex[:12]
    tel = f"19{token[:9]}"
    payload = register_request(tel, "qa_password", f"split_{token}")
    frame = pack_frame(payload)
    client = ProtocolClient().connect()
    try:
        client.sock.sendall(frame[:2])
        client.sock.sendall(frame[2:9])
        client.sock.sendall(frame[9:])
        response = recv_frame(client.sock)
        assert result_response(response, PACK_REGISTER_RS) == 1
    finally:
        client.close()
