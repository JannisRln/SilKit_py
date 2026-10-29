# Participants and configuration API

Import the public API with `import SilKit_py as silkit`.

## Module functions

| Function | Signature | Description |
| --- | --- | --- |
| `participant_configuration_from_String` | `(string: str) -> IParticipantConfiguration` | Parses SIL Kit configuration text (typically YAML). The mixed-case function name is intentional. |
| `create_participant` | `(participant_config: IParticipantConfiguration, participant_name: str, registry_uri: str = "silkit://localhost:8500") -> IParticipant` | Creates a participant connecting to a SIL Kit registry. |
| `__version__` | `str` | Installed extension version. |

## IParticipant methods

| Method | Parameters | Returns |
| --- | --- | --- |
| `create_lifecycle_service` | `lifecycle_service_config: LifecycleConfiguration` | `ILifecycleService` |
| `create_can_controller` | `canonical_name: str, network_name: str` | `ICanController` |
| `create_data_publisher` | `canonical_name: str, data_spec: PubSubSpec, history: int = 0` | `IDataPublisher` |
| `create_data_subscriber` | `canonical_name: str, data_spec: PubSubSpec, data_message_handler: Callable[[IDataSubscriber, DataMessageEvent], None]` | `IDataSubscriber` |
| `create_rpc_client` | `canonical_name: str, data_spec: RpcSpec, handler: Callable[[IRpcClient, RpcCallResultEvent], None]` | `IRpcClient` |
| `create_rpc_server` | `canonical_name: str, data_spec: RpcSpec, handler: Callable[[IRpcServer, RpcCallEvent], None]` | `IRpcServer` |
| `get_logger` | no arguments | `ILogger` |

Callbacks are ordinary synchronous Python functions. Create the participant once, then create its services before starting the lifecycle. The C++ service objects returned are associated with their owning participant; keep the participant alive while using them.

```python
import SilKit_py as silkit

config = silkit.participant_configuration_from_String("")
participant = silkit.create_participant(
    config, "ExampleParticipant", "silkit://localhost:8500"
)
logger = participant.get_logger()
logger.log(silkit.LogLevel.Info, "Participant created")
```

**Source of truth:** [main.cpp](https://github.com/JannisRln/SilKit_py/blob/main/src/main.cpp) and the [Python type stub](https://github.com/JannisRln/SilKit_py/blob/main/src/SilKit_py/__init__.pyi).
