"""Launch the SIL Kit utilities bundled with this wheel."""

from __future__ import annotations

import subprocess
import sys
from importlib.resources import files
from pathlib import Path


def executable(name: str) -> Path:
    """Return the absolute path to a bundled SIL Kit utility."""
    suffix = ".exe" if sys.platform == "win32" else ""
    path = Path(str(files("SilKit_py").joinpath("bin", f"{name}{suffix}")))
    if not path.is_file():
        msg = f"Bundled SIL Kit utility not found: {path}"
        raise FileNotFoundError(msg)
    return path


def run(name: str) -> int:
    """Run a bundled utility, forwarding command-line arguments and exit code."""
    return subprocess.call([str(executable(name)), *sys.argv[1:]])


def registry() -> int:
    return run("sil-kit-registry")


def system_controller() -> int:
    return run("sil-kit-system-controller")


def monitor() -> int:
    return run("sil-kit-monitor")
