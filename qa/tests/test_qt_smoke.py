import os
from pathlib import Path
import subprocess

import pytest


@pytest.mark.smoke
def test_qt_project_files_are_present():
    repo = Path(__file__).resolve().parents[2]
    assert (repo / "WeChat" / "WeChat.pro").is_file()
    assert (repo / "WeChat" / "resource.qrc").is_file()


@pytest.mark.smoke
def test_qt_client_can_start_when_configured():
    executable = os.getenv("MEETING_QT_EXE")
    if not executable:
        pytest.skip("set MEETING_QT_EXE to enable the optional Windows Qt smoke test")
    process = subprocess.Popen([executable], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    try:
        assert process.poll() is None
    finally:
        process.terminate()
        process.wait(timeout=5)
