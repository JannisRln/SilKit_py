# Publish/subscribe API

## PubSubSpec

`PubSubSpec()` or `PubSubSpec(topic: str, mediaType: str)` defines a topic and media type.

| Member | Description |
| --- | --- |
| `topic` | Read-only topic name. |
| `media_type` | Read-only media type. |
| `labels` | Read-only matching labels. |
| `add_label(label: MatchingLabel)` | Adds a label. |
| `add_label(key: str, value: str, kind: MatchingLabelKind)` | Adds a label from components. |

`MatchingLabel(key, value, kind)` has writable `key`, `value`, and `kind` fields. `MatchingLabelKind` has `Optional` and `Mandatory` values. Matching labels are also used by RPC specifications.

## Data services

| Class/member | Behavior |
| --- | --- |
| `IParticipant.create_data_publisher(name, spec, history=0)` | Creates publisher. |
| `IDataPublisher.publish(data: bytes)` | Publishes raw bytes; encode your payload first. |
| `IParticipant.create_data_subscriber(name, spec, handler)` | Creates subscriber; `handler(subscriber, event)` is synchronous. |
| `IDataSubscriber` | Subscriber handle; no public instance methods are bound. |
| `DataMessageEvent.timestamp` | Message timestamp **in nanoseconds**. |
| `DataMessageEvent.data` | Received `bytes` payload. |
| `media_type_data()` | Upstream SIL Kit serializer media-type helper. |

```python
import SilKit_py as silkit

spec = silkit.PubSubSpec("speed", "application/octet-stream")
publisher = participant.create_data_publisher("SpeedPublisher", spec)

def on_message(subscriber, event):
    print("timestamp ns:", event.timestamp, "payload:", event.data)

subscriber = participant.create_data_subscriber("SpeedSubscriber", spec, on_message)
publisher.publish((42).to_bytes(4, "little", signed=True))
```

This excerpt assumes `participant` has already been created and its lifecycle is managed by the application. Subscribers and publishers must agree on topic, media type, labels and wire encoding.

**Source:** [PubSub.hpp](https://github.com/JannisRln/SilKit_py/blob/main/src/PubSub.hpp).
