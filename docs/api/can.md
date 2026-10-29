# CAN API

The Python binding supports a subset of SIL Kit CAN services. The controller uses the network name specified when created.

## ICanController

| Method | Signature / behavior |
| --- | --- |
| `set_baud_rate(rate, fd_rate, xl_rate)` | Configures classical, FD, and XL rates. |
| `reset()` | Resets the controller. |
| `start()` | Starts the controller. |
| `stop()` | Stops the controller. |
| `sleep()` | Puts the controller into sleep state. |
| `send_frame(can_id, data, flags=0, dlc=None, sdt=0, vcid=0, af=0, user_context=0)` | Sends a frame with `bytes` payload. If `dlc=None`, it is inferred from payload length and frame flags. |
| `add_frame_handler(handler, direction_mask=TransmitDirection.RX)` | Registers `handler(controller, event)`, returns a handler ID. |

### Frame properties

`CanFrame`: read-only `can_id`, `flags`, `dlc`, and `data: bytes` in received events.

`CanFrameEvent`: read-only `timestamp` (**nanoseconds**), `frame`, `direction`, `user_context`.

### Enumerations and constants

- `CanFrameFlag`: `Ide`, `Rtr`, `Fdf`, `Brs`, `Esi`, `Xlf`, `Sec`.
- `TransmitDirection`: `Undefined`, `RX`, `TX`, `TXRX`.
- `CanControllerState`: `Uninit`, `Stopped`, `Started`, `Sleep`.
- `CanErrorState`: `NotAvailable`, `ErrorActive`, `ErrorPassive`, `BusOff`.
- `CanTransmitStatus`: `Transmitted`, `Canceled`, `TransmitQueueFull`.
- `CanTransmitStatus_DefaultMask`: default transmit-status mask constant.

### Sending and receiving

```python
controller = participant.create_can_controller("ExampleCAN", "CAN 1")

def on_frame(ctrl, event):
    print(event.timestamp, hex(event.frame.can_id), event.frame.data)

handler_id = controller.add_frame_handler(
    on_frame, silkit.TransmitDirection.RX
)
controller.start()
controller.send_frame(0x123, b"\x01\x02")
```

The excerpt assumes `participant` is created and `SilKit_py` is imported as `silkit`; the lifecycle must also be started.

For auto-inferred DLC values, the binding accepts payload lengths: classical CAN **0–8 bytes**, CAN FD **0–8, 12, 16, 20, 24, 32, 48, 64 bytes**, and CAN XL **1–2048 bytes**. Invalid sizes raise `ValueError`; payloads are not padded. Frame flags must match the chosen CAN format.

**Source:** [ICanController.hpp](https://github.com/JannisRln/SilKit_py/blob/main/src/ICanController.hpp).
