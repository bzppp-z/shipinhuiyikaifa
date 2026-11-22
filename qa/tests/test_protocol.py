import socket
import struct

import pytest

from protocol_client.framing import pack_frame, recv_frame


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
