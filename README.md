# SilKit_py

**Python bindings for [Vector SIL Kit](https://github.com/vectorgrp/sil-kit)**

`SilKit_py` exposes SIL Kit functionality to Python applications through a native extension built with [pybind11](https://pybind11.readthedocs.io/). It can be used to implement simulation participants, exchange data with other SIL Kit participants, and write automated simulation tests. For higher-level scenario-based testing, see the separate [SIL Kit Scenario Runner](https://github.com/JannisRln/SilKit_py_scenario_runner) project.

> **Project status:** Experimental. The Python API is still evolving and does not yet cover every SIL Kit service.

## Features

- Participant creation and configuration
- Autonomous and coordinated lifecycle services
- Time synchronization with simulation-step callbacks
- Publish/subscribe data communication
- CAN communication (partial API coverage)
- RPC client/server functionality
- SIL Kit logging
- Awaitable lifecycle startup via Python `asyncio` (`start_lifecycle()`)
- Command-line entry points for the SIL Kit registry, system controller, and monitor

For details on the underlying framework, see the [SIL Kit documentation](https://vectorgrp.github.io/sil-kit-docs/index.html).

## Scenario-based testing (separate project)

For automated simulation scenarios, see **[SilKit_py_scenario_runner](https://github.com/JannisRln/SilKit_py_scenario_runner)**. This companion project builds on `SilKit_py` and provides higher-level utilities for:

- Scheduling actions at simulated times
- Exchanging and checking simulated signals
- Scenario assertions and expectation handling
- CAN/UDS testing helpers and `pytest`-based integration tests

The runner is **maintained in its own repository and distributed as a separate Python package**. It is not bundled with `SilKit_py` and is **not required** to use the core SIL Kit bindings.

Once the runner has been published to PyPI under the name currently declared in its `pyproject.toml`, install it separately:

```bash
python -m pip install silkit-scenario-runner
```

For setup, examples, and test-environment requirements, see the [Scenario Runner README](https://github.com/JannisRln/SilKit_py_scenario_runner#readme). Its integration tests require a suitable SIL Kit registry and peer participants; `SilKit_py` alone does not supply an ECU implementation.

## Installation

Once version **1.0.5** is published to PyPI, install it with:

```bash
python -m pip install SilKit_py==1.0.5
```

Verify the installation:

```bash
python -c "import SilKit_py as silkit; print(silkit.__version__)"
```

### Python and platform support

- **CPython 3.12–3.14:** targeted by the current CI and wheel-building configuration.
- **Windows x64 and Linux x86_64:** targeted for prebuilt release wheels.
- **macOS:** included in source-build CI testing, but no prebuilt macOS wheel is promised.
- **Python 3.9–3.11:** allowed by the current package metadata, but not covered by the current release wheel matrix.

Check the **Download files** section of the [PyPI project](https://pypi.org/project/SilKit-py/) after release for the wheel files actually available. If no compatible wheel is available, pip may attempt a source build, which requires a working C++ toolchain and the SIL Kit sources.

## SIL Kit command-line tools

The package defines these command-line entry points:

```bash
silkit-registry
silkit-system-controller
silkit-monitor
```

For wheels built with the project's current packaging configuration, the native SIL Kit tools are bundled with the package. Start the registry in a separate terminal before running the example below:

```bash
silkit-registry
```

By default, the example connects to `silkit://localhost:8501`.

## Quick start

The following participant uses autonomous lifecycle mode and a simulation-step callback:

```python
import asyncio

import SilKit_py as silkit


configuration = silkit.participant_configuration_from_String("""
Logging:
  Sinks:
    - Type: Stdout
      Level: Info
""")

participant = silkit.create_participant(
    configuration,
    "PythonParticipant",
    "silkit://localhost:8501",
)

lifecycle = participant.create_lifecycle_service(
    silkit.LifecycleConfiguration(silkit.OperationMode.Autonomous)
)
time_sync = lifecycle.create_time_sync_service()


def simulation_step(now_ns: int, duration_ns: int) -> None:
    print(f"Simulation time: {now_ns} ns; step: {duration_ns} ns")


time_sync.set_simulation_step_handler(simulation_step, 1_000_000)  # 1 ms


async def main() -> None:
    await lifecycle.start_lifecycle()


if __name__ == "__main__":
    asyncio.run(main())
```

`start_lifecycle()` returns an awaitable: the binding uses `asyncio.to_thread()` to wait for the SIL Kit lifecycle without blocking the Python event loop. The simulation-step callback above is a regular synchronous function, not an `async` callback. The lifecycle remains active until the simulation is stopped. When using coordinated mode, the simulation also needs a system controller and the appropriate participants.

### CAN frame notes

For the CAN API, `send_frame(..., dlc=None)` determines the DLC from the payload length. Supported payload lengths are:

- Classic CAN: 0–8 bytes
- CAN FD: 0–8, 12, 16, 20, 24, 32, 48, or 64 bytes
- CAN XL: 1–2048 bytes

Invalid payload lengths raise `ValueError`; payloads are not padded automatically. `CanFrameEvent.timestamp` is expressed in nanoseconds.

## Building from source

### Requirements

- Git (including Git submodule support)
- CMake
- A compatible C++ compiler (MSVC on Windows or GCC/Clang on Linux)
- Python development headers, as required by your platform
- Python and pip

Clone the repository with its SIL Kit submodule:

```bash
git clone --recursive https://github.com/JannisRln/SilKit_py.git
cd SilKit_py
```

If you cloned without `--recursive`, initialize the submodule separately:

```bash
git submodule update --init --recursive
```

Create a virtual environment:

**Windows (PowerShell)**

```powershell
py -3.12 -m venv .venv
.venv\Scripts\Activate.ps1
```

**Linux / macOS**

```bash
python3 -m venv .venv
source .venv/bin/activate
```

Install and build from the repository root:

```bash
python -m pip install --upgrade pip
python -m pip install . --group dev
```

The `dev` dependency group includes the test dependencies and build/development tools. Dependency groups require **pip 25.1 or newer**.

To create a source distribution and wheel explicitly:

```bash
python -m build
```

Build artifacts appear in `dist/`. The native SIL Kit build uses the project's CMake configuration and its `vendor/sil-kit` submodule.

## Running tests

After installing from source with the `dev` group:

```bash
python -m pytest
```

The test suite covers participant setup, lifecycle behavior, time synchronization, data communication, and supported service bindings. Some tests launch local SIL Kit utilities. If you need to point the tests to another utility directory, set `SILKIT_BIN_DIR` as documented by the test setup.

## CI and releases

The repository has two separate GitHub Actions workflows:

| Workflow | Purpose |
| --- | --- |
| `ci.yml` | Build and run tests on pull requests and pushes to `main` |
| `wheels.yml` | Build and verify source/wheel distributions; publish to PyPI on a GitHub release |

Publishing uses **PyPI Trusted Publishing** with the GitHub Environment `pypi` and the registered workflow file `wheels.yml`.

For a **1.0.5** release:

1. Confirm that `pyproject.toml` contains `version = "1.0.5"`.
2. Ensure the CI and distribution builds pass on the supported targets.
3. Create and publish a GitHub Release with tag `v1.0.5`.
4. Verify the generated artifacts and installation from PyPI.

## API coverage and roadmap

| Service | Status |
| --- | --- |
| Participant configuration and creation | Available |
| Lifecycle services | Available; some handlers are not implemented |
| Time synchronization | Available; asynchronous step handler not implemented |
| Pub/Sub | Available |
| Logging | Available |
| CAN | Partially implemented |
| RPC | Available |
| LIN, FlexRay, Ethernet | Not yet implemented |
| Data serialization/deserialization helpers | Not yet implemented |
| System utilities and custom network simulation APIs | Not yet implemented |

The coverage above describes the Python binding, not the feature set of the underlying C++ SIL Kit library.

## Related projects

- [Vector SIL Kit](https://github.com/vectorgrp/sil-kit) — underlying simulation and communication framework
- [SIL Kit documentation](https://vectorgrp.github.io/sil-kit-docs/index.html) — framework and API documentation
- [SilKit_py_scenario_runner](https://github.com/JannisRln/SilKit_py_scenario_runner) — optional ECU-independent scenario scheduling, signal assertions, CAN/UDS helpers, and pytest-based integration tests

## License

`SilKit_py` is distributed under the [MIT License](LICENSE). SIL Kit and bundled third-party components retain their respective licenses and notices; consult the license files included with the distribution and the upstream SIL Kit project.
