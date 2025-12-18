from __future__ import annotations

import SilKit_py as m
import asyncio
import threading
import subprocess, time
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

Dashboard_active = True

@pytest.fixture(scope="session", autouse=True)
def silkit_registry():

    global uri

    cmd = [
        "./build-silkit/Release/sil-kit-registry.exe",
        "--listen-uri", uri,
        "-l", "Off",
    ]

    if Dashboard_active:
        cmd.extend([
            "--dashboard-uri", "http://localhost:8082",
            "--enable-dashboard",
        ])

    proc = subprocess.Popen(cmd)

    time.sleep(0.5)  # wait for registry to start
    yield
    proc.terminate()
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()