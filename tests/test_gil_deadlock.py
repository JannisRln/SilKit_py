from __future__ import annotations

import gc
import subprocess
import sys
import threading
import weakref
from functools import partial
from pathlib import Path

import pytest
from common import simpel_yamel, uri

import SilKit_py as m


@pytest.mark.usefixtures("silkit_registry")
@pytest.mark.parametrize(
    "operation",
    [
        "publisher",
        "subscriber",
        "can",
        "rpc_client",
        "rpc_server",
        "lifecycle",
        "time_sync",
        "destroy",
        "stress",
        "callbacks",
        "exit",
    ],
)
def test_native_calls_allow_python_callbacks(operation):
    # An in-process timeout cannot interrupt a native call holding the GIL.
    # Isolate the scenario so a regression fails instead of hanging pytest.
    result = subprocess.run(
        [sys.executable, str(Path(__file__).resolve()), operation],
        capture_output=True,
        text=True,
        timeout=30,
        check=False,
    )
    assert result.returncode == 0, result.stdout + result.stderr


def _participant(name):
    return m.create_participant(
        m.participant_configuration_from_String(simpel_yamel), name, uri
    )


def _ignore(*_args):
    pass


def _create_endpoint(participant, operation, index=0):
    name = f"{operation}_{index}"
    data_spec = m.PubSubSpec("GILRegression", "application/octet-stream")
    rpc_spec = m.RpcSpec("GILRegression", "application/octet-stream")
    factories = {
        "publisher": lambda: participant.create_data_publisher(name, data_spec),
        "subscriber": lambda: participant.create_data_subscriber(
            name, data_spec, _ignore
        ),
        "can": lambda: participant.create_can_controller(name, "CAN1"),
        "rpc_client": lambda: participant.create_rpc_client(name, rpc_spec, _ignore),
        "rpc_server": lambda: participant.create_rpc_server(name, rpc_spec, _ignore),
        "lifecycle": lambda: participant.create_lifecycle_service(
            m.LifecycleConfiguration(m.OperationMode.Autonomous)
        ),
    }
    return factories[operation]()


def _blocked_callback(operation):
    sender = _participant("GILSender")
    receiver = _participant("GILReceiver")
    tx = sender.create_can_controller("Tx", "CAN1")
    rx = receiver.create_can_controller("Rx", "CAN1")
    entered = threading.Event()
    resume = threading.Event()
    finished = threading.Event()

    def on_frame(_controller, _event):
        entered.set()
        # Keep SIL Kit's I/O thread in a Python callback until the main thread
        # is about to make a native call that needs that same I/O thread.
        assert resume.wait(10)
        finished.set()

    rx.add_frame_handler(on_frame)
    tx.start()
    rx.start()
    if operation == "time_sync":
        lifecycle = receiver.create_lifecycle_service(
            m.LifecycleConfiguration(m.OperationMode.Autonomous)
        )

    tx.send_frame(0x123, b"GIL")
    assert entered.wait(5), "CAN callback did not start"

    previous_interval = sys.getswitchinterval()
    try:
        # Do not let a periodic Python thread switch release the GIL between
        # waking the callback and entering the binding. The binding must do it.
        sys.setswitchinterval(10)
        resume.set()
        if operation == "destroy":
            del rx
            del receiver
        elif operation == "time_sync":
            service = lifecycle.create_time_sync_service()
            assert service is not None
        else:
            service = _create_endpoint(receiver, operation)
            assert service is not None
    finally:
        sys.setswitchinterval(previous_interval)

    assert finished.wait(5), "CAN callback could not reacquire the GIL"


def _connect_under_traffic():
    sender = _participant("GILTraffic")
    tx = sender.create_can_controller("Tx", "CAN1")
    tx.start()
    stop = threading.Event()

    def send():
        while not stop.is_set():
            tx.send_frame(0x123, b"traffic")
            stop.wait(0.001)

    thread = threading.Thread(target=send, daemon=True)
    thread.start()
    try:
        for index in range(50):
            participant = _participant(f"GILConnect_{index}")
            can = participant.create_can_controller("Rx", "CAN1")
            received = threading.Event()

            def on_frame(_controller, _event, received=received):
                received.set()

            can.add_frame_handler(on_frame)
            can.start()
            assert received.wait(5), f"No CAN traffic on connection {index}"
            endpoints = [
                _create_endpoint(participant, kind, endpoint_index)
                for endpoint_index in range(5)
                for kind in ("publisher", "subscriber")
            ]
            for publisher in endpoints[::2]:
                publisher.publish(b"payload")
            can.stop()
            del endpoints, can, participant
    finally:
        stop.set()
        thread.join(timeout=5)
    assert not thread.is_alive()


def _callback_ownership():
    participant = _participant("GILOwnership")
    spec = m.RpcSpec("Ownership", "application/octet-stream")

    class Handler:
        def __call__(self, *_args):
            pass

    def register(factory):
        handler = Handler()
        reference = weakref.ref(handler)
        service = factory(handler)
        return service, reference

    def check_failed_creation(factory):
        handler = Handler()
        reference = weakref.ref(handler)
        with pytest.raises(RuntimeError, match="non-empty"):
            factory(handler)
        del handler
        assert reference() is None

    # A throwing native factory must also dispose its temporary callback with
    # the GIL held and restore the GIL before translating the exception.
    check_failed_creation(partial(participant.create_rpc_client, "", spec))
    check_failed_creation(partial(participant.create_rpc_server, "", spec))
    check_failed_creation(
        partial(
            participant.create_data_subscriber,
            "",
            m.PubSubSpec("Ownership", "application/octet-stream"),
        )
    )

    client, client_handler = register(
        partial(participant.create_rpc_client, "Client", spec)
    )
    server, server_handler = register(
        partial(participant.create_rpc_server, "Server", spec)
    )
    subscriber, subscriber_handler = register(
        partial(
            participant.create_data_subscriber,
            "Subscriber",
            m.PubSubSpec("Ownership", "application/octet-stream"),
        )
    )
    can = participant.create_can_controller("CAN", "CAN1")
    _, frame_handler = register(can.add_frame_handler)
    lifecycle = participant.create_lifecycle_service(
        m.LifecycleConfiguration(m.OperationMode.Autonomous)
    )
    _, ready_handler = register(lifecycle.set_communication_ready_handler)
    time_sync = lifecycle.create_time_sync_service()
    _, step_handler = register(
        partial(time_sync.set_simulation_step_handler, initial_step_size=1_000_000)
    )

    references = [
        client_handler,
        server_handler,
        subscriber_handler,
        frame_handler,
        ready_handler,
        step_handler,
    ]
    gc.collect()
    assert all(reference() is not None for reference in references)
    client.set_call_result_handler(Handler())
    server.set_call_handler(Handler())
    lifecycle.set_communication_ready_handler(Handler())
    time_sync.set_simulation_step_handler(Handler(), 1_000_000)
    assert all(
        reference() is None
        for reference in (client_handler, server_handler, ready_handler, step_handler)
    )

    errors = []
    reported = threading.Event()

    def on_unraisable(error):
        errors.append((error.exc_type, str(error.exc_value)))
        reported.set()

    def failing_handler(*_args):
        message = "callback failure"
        raise ValueError(message)

    previous_hook = sys.unraisablehook
    try:
        sys.unraisablehook = on_unraisable
        can.add_frame_handler(failing_handler, m.TransmitDirection.TX)
        can.start()
        can.send_frame(0x123, b"error")
        assert reported.wait(5)
        assert errors == [(ValueError, "callback failure")]
    finally:
        sys.unraisablehook = previous_hook

    # Native destruction releases the GIL and disposes retained callbacks.
    del client, server, subscriber, can, time_sync, lifecycle, participant
    gc.collect()
    assert all(reference() is None for reference in references)


_kept_alive = []


def _exit_with_live_callbacks():
    # The participant outlives main(), so it is destroyed during interpreter
    # finalization while SIL Kit keeps invoking the step handler. A native
    # thread that waits for the GIL at that point never returns, and the
    # participant destructor would join it forever.
    participant = _participant("GILExit")
    lifecycle = participant.create_lifecycle_service(
        m.LifecycleConfiguration(m.OperationMode.Autonomous)
    )
    time_sync = lifecycle.create_time_sync_service()
    steps = threading.Semaphore(0)

    def on_step(_now, _duration):
        steps.release()

    time_sync.set_simulation_step_handler(on_step, 1_000_000)
    waiting = lifecycle.start_lifecycle()
    waiting.close()  # The simulation keeps running; only the wait is dropped.
    for _ in range(20):
        assert steps.acquire(timeout=5), "Simulation steps did not start"
    _kept_alive.extend([participant, lifecycle, time_sync, on_step])


if __name__ == "__main__":
    operation = sys.argv[1]
    if operation == "exit":
        _exit_with_live_callbacks()
    elif operation == "stress":
        _connect_under_traffic()
    elif operation == "callbacks":
        _callback_ownership()
    else:
        _blocked_callback(operation)
