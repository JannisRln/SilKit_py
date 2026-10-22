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

## Installation

```bash
python -m pip install SilKit_py
```

Prebuilt wheels are available for CPython 3.12–3.14 on Windows (AMD64) and
Linux (x86_64). They already contain the SIL Kit runtime and tools, so nothing
else needs to be installed. On other platforms pip builds from source, which
requires a C++ compiler (see [Development](#development)).

## SIL Kit tools

The registry, system controller, and monitor are included in the package and
can be started from any directory:

```bash
silkit-registry
silkit-system-controller
silkit-monitor
```

## Example

Start `silkit-registry` first, then:

```python
import asyncio
import SilKit_py as m

cfg = m.participant_configuration_from_String("""
Logging:
  Sinks:
  - Type: Stdout
    Level: Info
""")
participant = m.create_participant(cfg, "PyParticipant", "silkit://localhost:8501")

lifecycle = participant.create_lifecycle_service(
    m.LifecycleConfiguration(m.OperationMode.Autonomous)
)
time_sync = lifecycle.create_time_sync_service()

def step_handler(now_ns: int, duration_ns: int) -> None:
    print(f"now={now_ns} ns, dt={duration_ns} ns")

time_sync.set_simulation_step_handler(step_handler, 1_000_000)  # 1 ms

asyncio.run(lifecycle.start_lifecycle())
```

See the [SIL Kit documentation](https://vectorgrp.github.io/sil-kit-docs/index.html)
for the underlying C++ API.

**CAN notes:** with `send_frame(..., dlc=None)` the DLC is derived from the
payload length. Valid lengths are 0–8 bytes for classic CAN, 0–8, 12, 16, 20,
24, 32, 48, or 64 bytes for CAN FD, and 1–2048 bytes for CAN XL. Other lengths
raise `ValueError`, and payloads are never padded. `CanFrameEvent.timestamp`
is given in nanoseconds.

## Development

Requirements: Python ≥ 3.9, CMake, and a C++ compiler (Visual Studio Build
Tools with the C++ workload on Windows, GCC/Clang plus Python headers on Linux).

```bash
git clone --recursive <this-repo-url>
cd SilKit_py
python -m venv .venv
.venv\Scripts\activate          # Linux: source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install . --group dev
python -m pytest
```

The first build compiles SIL Kit into `build-silkit/`; later builds reuse it.
To create a wheel in `dist/`, run `python -m build --wheel`.

- Dependency groups in `pyproject.toml` (pip ≥ 25.1 required): `test` contains
  pytest; `dev` also adds `build`, Ruff, and pybind11 headers for the IDE.
- The tests start the registry themselves and use the tools from
  `build-silkit/Release`. Set `SILKIT_BIN_DIR` to use another location.
- The VS Code tasks `Create Package` and `Install Package` use the local `.venv`.

## CI and releases

- **`ci.yml`:** runs on pull requests and pushes to `main`. It builds the
  package and runs the tests on Windows, Linux, and macOS with Python
  3.12–3.14. It never publishes anything.
- **`wheels.yml`:** builds the sdist and the wheels (via cibuildwheel) and
  tests them. When a GitHub Release is published, it uploads them to PyPI.

To release, increase `version` in `pyproject.toml`, then publish a GitHub
Release. Publishing uses PyPI Trusted Publishing with workflow `wheels.yml`
and environment `pypi`.

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
