# RPC API

RPC uses raw `bytes` arguments and responses. The application is responsible for serialization, matching function names and media types, and responding to requests.

## RpcSpec

`RpcSpec()` or `RpcSpec(function_name: str, mediaType: str)`.

- Read-only: `function_name`, `media_type`, `labels`.
- `add_label(label: MatchingLabel)` or `add_label(key, value, kind)`.
- `media_type_rpc()` returns the upstream SIL Kit RPC media type helper.

## IRpcClient

| Method | Description |
| --- | --- |
| `call(data: bytes, user_context: int | None = None)` | Starts an asynchronous RPC. |
| `call_with_timeout(data: bytes, timeout_ns: int, user_context: int | None = None)` | Starts a call with timeout in **nanoseconds**. |
| `set_call_result_handler(handler)` | Registers `handler(client, event)`. |

`RpcCallResultEvent`: `timestamp_ns`, `user_context`, `call_status`, and `result_data: bytes`. The result payload is valid when `call_status == RpcCallStatus.Success`.

`RpcCallStatus` values: `Success`, `ServerNotReachable`, `UndefinedError`, `InternalServerError`, `Timeout`.

## IRpcServer

| Method | Description |
| --- | --- |
| `submit_result(call_handle: int, result_data: bytes)` | Replies to a specific call. |
| `set_call_handler(handler)` | Registers `handler(server, event)`. |

`RpcCallEvent`: `timestamp_ns` (**nanoseconds**), `call_handle` (opaque integer used to reply), and `argument_data: bytes`.

## Endpoints

```python
spec = silkit.RpcSpec("echo", "application/octet-stream")

def on_call(server, event):
    server.submit_result(event.call_handle, event.argument_data)

def on_result(client, event):
    if event.call_status == silkit.RpcCallStatus.Success:
        print(event.result_data)

server = participant.create_rpc_server("EchoServer", spec, on_call)
client = participant.create_rpc_client("EchoClient", spec, on_result)
client.call(b"hello")
```

The excerpt assumes `participant` exists, `SilKit_py` is imported as `silkit`, and the simulation lifecycle is running. Callbacks are synchronous Python functions. User contexts are passed through as integer/pointer-like values, not Python objects.

**Sources:** [PubSub.hpp](https://github.com/JannisRln/SilKit_py/blob/main/src/PubSub.hpp) and [main.cpp](https://github.com/JannisRln/SilKit_py/blob/main/src/main.cpp).
