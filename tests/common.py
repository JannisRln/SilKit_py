from __future__ import annotations

import os
from pathlib import Path
import socket
import subprocess
import sys
import time
from urllib.parse import urlparse
import pytest

simpel_yamel = \
"""
Logging:
  Sinks:
  - Type: Stdout
    Level: Off
    LogName: SimpleParticipantLog
    HealthCheck:
      SoftResponseTimeout: 500
      HardResponseTimeout: 1000
"""

uri= "silkit://localhost:8501"

Dashboard_active = False


def silkit_executable(name: str) -> str:
    """Resolve a native SIL Kit tool independently of the working directory.

    SILKIT_BIN_DIR can point to a separately built or installed tool directory.
    """
    default_dir = Path(__file__).resolve().parents[1] / "build-silkit" / "Release"
    binary_dir = Path(os.environ.get("SILKIT_BIN_DIR", default_dir)).resolve()
    suffix = ".exe" if sys.platform == "win32" else ""
    executable = binary_dir / (name + suffix)
    if not executable.is_file():
        raise FileNotFoundError(
            f"SIL Kit tool not found: {executable}. Build SIL Kit with utilities "
            "enabled or set SILKIT_BIN_DIR."
        )
    return str(executable)


@pytest.fixture
def silkit_registry():

    global uri

    cmd = [
        silkit_executable("sil-kit-registry"),
        "--listen-uri", uri,
        "-l", "Off",
    ]

    if Dashboard_active:
        cmd.extend([
            "--dashboard-uri", "http://localhost:8082",
            "--enable-dashboard",
        ])

    proc = subprocess.Popen(cmd)

    try:
        address = urlparse(uri)
        deadline = time.monotonic() + 10
        while True:
            if proc.poll() is not None:
                raise RuntimeError(f"SIL Kit registry exited with code {proc.returncode}")
            try:
                with socket.create_connection((address.hostname, address.port), timeout=0.2):
                    break
            except OSError:
                if time.monotonic() >= deadline:
                    raise TimeoutError(f"SIL Kit registry did not start at {uri}")
                time.sleep(0.05)
        yield
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
