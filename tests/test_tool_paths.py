from __future__ import annotations

from pathlib import Path

import pytest

import common


@pytest.mark.parametrize("platform,suffix", [("win32", ".exe"), ("linux", "")])
def test_tool_path_from_other_working_directory(monkeypatch, tmp_path, platform, suffix):
    root = tmp_path / "project"
    tests = root / "tests"
    tests.mkdir(parents=True)
    binary_dir = root / "build-silkit" / "Release"
    binary_dir.mkdir(parents=True)
    executable = binary_dir / ("sil-kit-registry" + suffix)
    executable.touch()
    monkeypatch.setattr(common, "__file__", str(tests / "common.py"))
    monkeypatch.setattr(common.sys, "platform", platform)
    monkeypatch.delenv("SILKIT_BIN_DIR", raising=False)
    monkeypatch.chdir(tmp_path)
    assert Path(common.silkit_executable("sil-kit-registry")) == executable


def test_tool_directory_override(monkeypatch, tmp_path):
    monkeypatch.setenv("SILKIT_BIN_DIR", str(tmp_path))
    monkeypatch.setattr(common.sys, "platform", "linux")
    executable = tmp_path / "sil-kit-system-controller"
    executable.touch()
    assert Path(common.silkit_executable("sil-kit-system-controller")) == executable


def test_missing_tool_has_actionable_error(monkeypatch, tmp_path):
    monkeypatch.setenv("SILKIT_BIN_DIR", str(tmp_path))
    with pytest.raises(FileNotFoundError, match="SILKIT_BIN_DIR"):
        common.silkit_executable("sil-kit-registry")
