# Logging and tools

## ILogger

Retrieve a logger with `participant.get_logger()`.

`ILogger.log(level: LogLevel, msg: str) -> None` sends a message to the SIL Kit logging service.

`LogLevel` values: `Trace`, `Debug`, `Info`, `Warn`, `Error`, `Critical`, `Off`.

```python
logger = participant.get_logger()
logger.log(silkit.LogLevel.Info, "Simulation started")
```

The excerpt assumes `participant` exists and `SilKit_py` is imported as `silkit`.

## Command-line entry points

The Python package declares these console scripts:

| Command | Purpose |
| --- | --- |
| `silkit-registry` | Starts the SIL Kit registry executable. |
| `silkit-system-controller` | Starts the system controller executable. |
| `silkit-monitor` | Starts the monitor executable. |

Executables are bundled for compatible release wheel configurations. For manual launch arguments and coordinated lifecycle setup, refer to [Vector SIL Kit documentation](https://vectorgrp.github.io/sil-kit-docs/index.html).

**Source:** [pyproject.toml](https://github.com/JannisRln/SilKit_py/blob/main/pyproject.toml).
