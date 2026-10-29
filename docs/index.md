# SilKit_py

Python bindings for [Vector SIL Kit](https://github.com/vectorgrp/sil-kit), implemented with pybind11. Create simulation participants, configure lifecycle and time synchronization, exchange data over Pub/Sub or CAN, and call RPC services.

```{toctree}
:maxdepth: 2
:caption: Guides

getting-started
concepts
troubleshooting
```

```{toctree}
:maxdepth: 2
:caption: API reference

api/participants
api/lifecycle
api/pubsub
api/can
api/rpc
api/logging
```

The API pages describe the **Python bindings**, not the full upstream C++ SIL Kit API. Unsupported upstream services are not implied to be available in Python.

For higher-level scheduled test scenarios, see [SilKit_py_scenario_runner](https://github.com/JannisRln/SilKit_py_scenario_runner).
