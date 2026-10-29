# Lifecycle and time synchronization API

## Configuration and enums

`LifecycleConfiguration(operation_mode: OperationMode)` selects the participant lifecycle mode; `operation_mode` is a writable property.

`OperationMode` values: `Invalid`, `Coordinated`, `Autonomous`.

`ParticipantState` values: `Invalid`, `ServicesCreated`, `CommunicationInitializing`, `CommunicationInitialized`, `ReadyToRun`, `Running`, `Paused`, `Stopping`, `Stopped`, `Error`, `ShuttingDown`, `Shutdown`, `Aborting`.

## ILifecycleService

| Method | Signature / behavior |
| --- | --- |
| `set_communication_ready_handler(handler)` | Registers a synchronous `handler() -> None`. |
| `set_communication_ready_handler_async(handler)` | Registers the asynchronous-ready callback `handler() -> None` (the handler itself is a standard Python callable). |
| `complete_communication_ready_handler_async()` | Signals completion after the asynchronous-ready handler's external work finishes. |
| `start_lifecycle()` | Returns an `Awaitable[ParticipantState]`; await it from Python asyncio. |
| `stop(reason: str)` | Requests lifecycle shutdown. |
| `state()` | Returns the current `ParticipantState`. |
| `create_time_sync_service()` | Returns `ITimeSyncService`. |

With coordinated mode, a system controller and the expected participant set are required. The registry alone does not start a coordinated simulation. For the asynchronous-ready hook, call `complete_communication_ready_handler_async()` after readiness work is finished; otherwise startup may not progress.

## ITimeSyncService

`set_simulation_step_handler(handler: Callable[[int, int], None], initial_step_size: int) -> None`

- `handler(now_ns, duration_ns)`: synchronous function called at simulation steps.
- `now_ns`: current simulation timestamp **in nanoseconds**.
- `duration_ns`: simulation-step duration **in nanoseconds**.
- `initial_step_size`: requested initial step size **in nanoseconds**.

```python
import asyncio
import SilKit_py as silkit

participant = silkit.create_participant(
    silkit.participant_configuration_from_String(""),
    "TimerParticipant",
    "silkit://localhost:8500",
)
lifecycle = participant.create_lifecycle_service(
    silkit.LifecycleConfiguration(silkit.OperationMode.Autonomous)
)
time_sync = lifecycle.create_time_sync_service()

def on_step(now_ns: int, duration_ns: int) -> None:
    print(now_ns, duration_ns)
    if now_ns >= 10_000_000:
        lifecycle.stop("Example complete")

time_sync.set_simulation_step_handler(on_step, 1_000_000)
asyncio.run(lifecycle.start_lifecycle())
```

`start_lifecycle()` is awaitable; the simulation-step callback is **not** an `async def` coroutine. Exceptions from some bound callbacks are reported as unraisable errors by pybind11, rather than propagated to the awaiting application.

**Source:** [ILifecycleService.hpp](https://github.com/JannisRln/SilKit_py/blob/main/src/ILifecycleService.hpp).
