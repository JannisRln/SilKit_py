# SilKit_py

Python bindings for [Vector SIL Kit](https://github.com/vectorgrp/sil-kit/tree/main), providing access to core concepts such as:

- Participant configuration
- Lifecycle services (coordinated and autonomous)
- Time synchronization
- Pub/Sub data communication
- Logging

Bindings are implemented with [pybind11](https://pybind11.readthedocs.io/) and integrate with `asyncio`, so you can `await` lifecycle operations and write asynchronous tests using `pytest-asyncio`.

[Documentation of C++ API](https://vectorgrp.github.io/sil-kit-docs/index.html)

---

## Project Status

This repository is experimental / work-in-progress. API and build steps may change.

---

## Prerequisites

- Windows or Linux (64-bit)
- Python 3.12 (64-bit)
- C++ toolchain compatible with your Python installation (for building the extension)

---

## Getting the sources

Clone this repository and update the submodules:

```bash
git clone <this-repo-url>
cd SilKit_py
```

:: Update sub repos (submodules)
```bash
git submodule update --init --recursive
```

### Setup (Windows, cmd.exe)

All commands below assume you are in the repository root.

Create and activate a virtual environment:

```bash
python -m venv .venv
.venv\Scripts\activate.bat
```

Upgrade pip:
```bash
python -m pip install --upgrade pip
```

Build and install the package with its development tools:
```bash
python -m pip install . --group dev
```

To additionally create a distributable wheel in `dist/`:
```bash
python -m build --wheel
```

Python dependencies are maintained in `pyproject.toml`: `build-system.requires`
declares the isolated build dependencies, `test` contains the test tools, and
`dev` includes tests, the wheel builder, Ruff, and pybind11 headers for the IDE.
Transitive dependencies are installed automatically. Use `--group test` instead
of `--group dev` if you only need the tests, or `pip install --group dev` to install
the development tools without rebuilding the package. Dependency groups require
pip 25.1 or newer.

The VS Code tasks use the local `.venv` on Windows and Linux. `Create Package`
builds a wheel; `Install Package` installs the project into that environment.

After these steps, the SilKit_py package should be importable inside the virtual environment:
```bash
import SilKit_py as silkit
print(silkit.__version__)
```
Running the SIL Kit registry and tools

Some tests and examples assume the SIL Kit registry and tools are available as local executables, e.g.:
```bash
./build-silkit/Release/sil-kit-registry.exe

./build-silkit/Release/sil-kit-system-controller.exe

./build-silkit/Release/sil-kit-monitor.exe
```
They are also build while the wheel build

## Running the tests

With the virtual environment activated:
```bash
pytest -s
```

On Linux, create a virtual environment and build/install from the repository root:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install . --group dev
python -m pytest
```

The tests find SIL Kit tools in `build-silkit/Release` relative to the repository,
using `.exe` on Windows and no suffix on Linux. Set `SILKIT_BIN_DIR` to use tools
from another build or installation. The registry is started by the shared test
fixture; a dashboard is not required.

`ICanController.send_frame(..., dlc=None)` derives the wire DLC from the payload:
Classic CAN accepts 0–8 bytes; CAN FD accepts 0–8, 12, 16, 20, 24, 32, 48 or
64 bytes; CAN XL accepts 1–2048 bytes. Other lengths raise `ValueError` when
automatic DLC selection is used. Payloads are never padded implicitly. An explicit
`dlc` remains caller-controlled. Received `CanFrameEvent.timestamp` is an integer
number of nanoseconds.

The tests include:

Basic construction of participants and configurations

Lifecycle service usage (autonomous and coordinated)

Time sync with simulation step handlers

Pub/Sub publisher and subscriber interaction using the system controller

Minimal usage example
```Python
import SilKit_py as m

uri = "silkit://localhost:8501"

yaml_cfg = """
Logging:
  Sinks:
  - Type: File
    Level: Trace
    LogName: SimpleParticipantLog
"""

# Create participant configuration from YAML string
cfg = m.participant_configuration_from_String(yaml_cfg)

# Create a participant
participant = m.create_participant(cfg, "PyParticipant", uri)

# Create coordinated lifecycle and time sync
lc_cfg = m.LifecycleConfiguration(m.OperationMode.Autonomous)
lifecycle = participant.create_lifecycle_service(lc_cfg)
time_sync = lifecycle.create_time_sync_service()

def step_handler(now_ns: int, duration_ns: int) -> None:
    print(f"Simulation step: now={now_ns}, dt={duration_ns}")

time_sync.set_simulation_step_handler(step_handler, int(1_000_000))  # 1 ms

# Run lifecycle inside asyncio
import asyncio

async def main():
    await lifecycle.start_lifecycle()

asyncio.run(main())
```

# TODOs

Missing Implementations from SilKit
- [X] CAN Service API (not complete)
- [ ] LIN Service API
- [ ] FlexRay Service API
- [ ] Ethernet Service API

- [X] RPC (Remote Procedure Call) API
- [ ] Data Serialization/Deserialization API

ILifecycleService
- [ ] State machine handler

ITimeSyncService
- [ ] Asynchronous Step Handler

- [ ] System Utilities

- [ ] Custom Network Simulator
