# SilKit_py

Python bindings for Vector SIL Kit, providing access to core concepts such as:

- Participant configuration
- Lifecycle services (coordinated and autonomous)
- Time synchronization
- Pub/Sub data communication
- Logging

Bindings are implemented with [pybind11](https://pybind11.readthedocs.io/) and integrate with `asyncio`, so you can `await` lifecycle operations and write asynchronous tests using `pytest-asyncio`.

---

## Project Status

This repository is experimental / work-in-progress. API and build steps may change.

---

## Prerequisites

- Windows (64-bit)
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

Install Python dependencies:
```bash
pip install -r .\requirements.txt
```

Build the wheel and install it into the virtual environment:
```bash
.venv\Scripts\pip.exe install build ^
&& .venv\Scripts\python.exe -m build --wheel ^
&& .venv\Scripts\pip.exe install .\dist\silkit_py-1.0.0-cp312-cp312-win_amd64.whl --force-reinstall
```

Adjust the wheel file name if you change the version or Python build.

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
- [ ] CAN Service API
- [ ] LIN Service API
- [ ] FlexRay Service API
- [ ] Ethernet Service API

- [ ] RPC (Remote Procedure Call) API
- [ ] Data Serialization/Deserialization API

ILifecycleService
- [ ] State machine handler

ITimeSyncService
- [ ] Asynchronous Step Handler

- [ ] System Utilities

- [ ] Custom Network Simulator
