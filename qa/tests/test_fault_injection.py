"""Integration checks for the stage-eight fault probes."""

import pytest

from fault_injection import PROBES, run


pytestmark = pytest.mark.integration


@pytest.mark.parametrize("scenario", ["negative-length", "oversized-length", "unknown-packet", "truncated-known-packet", "partial-frame"])
def test_malformed_input_does_not_kill_listener(scenario: str):
    result = run([scenario])[0]
    assert result.ok, result.error


@pytest.mark.parametrize("scenario", ["abrupt-control-close", "abrupt-media-close", "duplicate-media-register", "illegal-room-id"])
def test_connection_lifecycle_fault_does_not_kill_listener(scenario: str):
    result = run([scenario])[0]
    assert result.ok, result.error
