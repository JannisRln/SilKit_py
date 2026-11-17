from __future__ import annotations

import asyncio
import gc
import subprocess
import sys
import threading
import weakref
from enum import IntFlag
from pathlib import Path

import pytest
from common import simpel_yamel, uri

import SilKit_py as m


@pytest.mark.usefixtures("silkit_registry")
@pytest.mark.parametrize(
    "scenario",
    [
        "owner_publisher",
        "owner_subscriber",
        "owner_can",
        "owner_client",
        "owner_server",
        "owner_logger",
        "owner_lifecycle",
        "owner_time_sync",
        "cycle_bound",
        "cycle_closure",
        "cycle_callable",
        "can_events",
        "pubsub_events",
        "rpc_events",
        "remove_handler",
        "cancel_autonomous",
        "cancel_coordinated",
        "pending_lifecycle",
    ],
)
def test_binding_lifetimes(scenario):
    # Native lifetime regressions can crash or hang; protect the pytest process.
    result = subprocess.run(
        [sys.executable, str(Path(__file__).resolve()), scenario],
        capture_output=True,
        text=True,
        timeout=30,
        check=False,
    )
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize(
    "enum_type", [m.CanFrameFlag, m.TransmitDirection, m.CanTransmitStatus]
)
def test_native_flags_support_bitmask_operations(enum_type):
    assert issubclass(enum_type, IntFlag)
    values = list(enum_type)
    combined = values[0] | values[1]
    assert isinstance(combined, enum_type)
    assert combined & values[0] == values[0]
    assert combined ^ values[0] == values[1]
    assert int(combined) == int(values[0]) | int(values[1])


def participant(name):
    return m.create_participant(
        m.participant_configuration_from_String(simpel_yamel), name, uri
    )


def noop(*_args):
    pass


def check_owner(kind):
    p = participant("Owner")
    data_spec = m.PubSubSpec("Topic", "application/octet-stream")
    rpc_spec = m.RpcSpec("Function", "application/octet-stream")
    if kind == "publisher":
        service = p.create_data_publisher("Pub", data_spec)
    elif kind == "subscriber":
        service = p.create_data_subscriber("Sub", data_spec, noop)
    elif kind == "can":
        service = p.create_can_controller("CAN", "CAN1")
    elif kind == "client":
        service = p.create_rpc_client("Client", rpc_spec, noop)
    elif kind == "server":
        service = p.create_rpc_server("Server", rpc_spec, noop)
    elif kind == "logger":
        service = p.get_logger()
    else:
        lifecycle = p.create_lifecycle_service(
            m.LifecycleConfiguration(m.OperationMode.Autonomous)
        )
        service = (
            lifecycle.create_time_sync_service() if kind == "time_sync" else lifecycle
        )
        del lifecycle

    ref = weakref.ref(p)
    del p
    gc.collect()
    assert ref() is not None
    if kind == "publisher":
        service.publish(b"still alive")
    elif kind == "can":
        service.start()
        service.send_frame(0x123, b"alive")
    elif kind == "logger":
        service.log(m.LogLevel.Info, "still alive")
    elif kind == "lifecycle":
        assert isinstance(service.state(), m.ParticipantState)
    del service
    gc.collect()
    assert ref() is None


def check_cycle(kind):
    class Connection:
        def __init__(self):
            self.p = participant("Cycle")
            self.can = self.p.create_can_controller("CAN", "CAN1")

        def callback(self, *_args):
            pass

        def __call__(self, *_args):
            pass

    def make_cycle():
        connection = Connection()
        if kind == "bound":
            handler = connection.callback
        elif kind == "closure":
            handler = lambda *args: connection.callback(*args)  # noqa: PLW0108 - exercise a closure
        else:
            handler = connection
        connection.can.add_frame_handler(handler)
        return weakref.ref(connection), weakref.ref(connection.p)

    connection_ref, participant_ref = make_cycle()
    gc.collect()
    assert connection_ref() is None
    assert participant_ref() is None
    # The registry name must be available again, not just the Python object.
    replacement = participant("Cycle")
    assert replacement is not None


def check_can_events():
    p = participant("CANEvents")
    can = p.create_can_controller("CAN", "CAN1")
    received = threading.Event()
    events = []

    def on_frame(_controller, event):
        events.append(event)
        received.set()

    can.add_frame_handler(on_frame, m.TransmitDirection.TX | m.TransmitDirection.RX)
    can.start()
    for index in range(100):
        received.clear()
        payload = bytes([index]) * 12 if index else b""
        can.send_frame(
            0x123, payload, m.CanFrameFlag.Fdf | m.CanFrameFlag.Brs, user_context=index
        )
        assert received.wait(5)
    del can, p
    gc.collect()
    assert len(events) == 100
    for index, event in enumerate(events):
        assert event.frame.data == (bytes([index]) * 12 if index else b"")
        assert event.user_context == index
        assert isinstance(event.user_context, int)
    frame = events[-1].frame
    events.clear()
    del event
    gc.collect()
    assert frame.data == bytes([99]) * 12


def check_pubsub_events():
    sender = participant("DataSender")
    receiver = participant("DataReceiver")
    spec = m.PubSubSpec("Topic", "application/octet-stream")
    publisher = sender.create_data_publisher("Pub", spec, 1)
    events = []
    received = threading.Event()

    def on_data(_subscriber, event):
        events.append(event)
        received.set()

    subscriber = receiver.create_data_subscriber("Sub", spec, on_data)
    # The participant retains handlers even without a Python service wrapper.
    del subscriber
    gc.collect()
    for index in range(50):
        received.clear()
        publisher.publish(bytes([index]) * (index + 1))
        assert received.wait(5)
    del publisher, sender, receiver
    gc.collect()
    assert [event.data for event in events] == [bytes([i]) * (i + 1) for i in range(50)]


def check_rpc_events():
    caller = participant("Caller")
    callee = participant("Callee")
    spec = m.RpcSpec("Function", "application/octet-stream")
    calls, results = [], []
    received = threading.Event()

    def on_call(server, event):
        calls.append(event)
        server.submit_result(event.call_handle, event.argument_data + b"reply")

    def on_result(_client, event):
        results.append(event)
        received.set()

    server = callee.create_rpc_server("Server", spec, on_call)
    client = caller.create_rpc_client("Client", spec, on_result)
    # Use lifecycle readiness to ensure RPC discovery has finished.
    caller_lc = caller.create_lifecycle_service(
        m.LifecycleConfiguration(m.OperationMode.Autonomous)
    )
    callee_lc = callee.create_lifecycle_service(
        m.LifecycleConfiguration(m.OperationMode.Autonomous)
    )

    async def exchange():
        waits = [
            asyncio.create_task(lc.start_lifecycle()) for lc in (caller_lc, callee_lc)
        ]
        try:
            for _ in range(1000):
                if all(
                    lc.state() == m.ParticipantState.Running
                    for lc in (caller_lc, callee_lc)
                ):
                    break
                await asyncio.sleep(0.005)
            for index in range(30):
                received.clear()
                client.call_with_timeout(bytes([index]) * index, 1_000_000_000, index)
                assert await asyncio.to_thread(received.wait, 5)
                assert results[-1].call_status == m.RpcCallStatus.Success
        finally:
            caller_lc.stop("done")
            callee_lc.stop("done")
            await asyncio.wait_for(asyncio.gather(*waits), 5)

    asyncio.run(exchange())
    client = caller_lc = callee_lc = None
    del server, caller, callee
    gc.collect()
    assert [e.argument_data for e in calls] == [bytes([i]) * i for i in range(30)]
    assert [e.result_data for e in results] == [
        bytes([i]) * i + b"reply" for i in range(30)
    ]
    assert [e.user_context for e in results] == list(range(30))


def check_remove_handler():
    p = participant("Remove")
    can = p.create_can_controller("CAN", "CAN1")

    class Handler:
        def __call__(self, *_args):
            pytest.fail("Removed callback was invoked")

    handler = Handler()
    ref = weakref.ref(handler)
    handler_id = can.add_frame_handler(handler, m.TransmitDirection.TX)
    del handler
    gc.collect()
    assert ref() is not None
    can.remove_frame_handler(handler_id)
    gc.collect()
    assert ref() is None
    can.start()
    can.send_frame(0x123, b"removed")


def check_cancellation(mode):
    p = participant("Cancellation")
    lifecycle = p.create_lifecycle_service(
        m.LifecycleConfiguration(
            m.OperationMode.Autonomous
            if mode == "autonomous"
            else m.OperationMode.Coordinated
        )
    )

    async def wait():
        with pytest.raises(TimeoutError):
            await asyncio.wait_for(lifecycle.start_lifecycle(), 0.05)

    asyncio.run(wait())
    # Reaching here proves the default executor is not stuck in future.get().
    if mode == "autonomous":
        lifecycle.stop("test finished")


def check_pending_lifecycle():
    p = participant("Pending")
    lifecycle = p.create_lifecycle_service(
        m.LifecycleConfiguration(m.OperationMode.Autonomous)
    )
    ref = weakref.ref(p)
    time_sync = lifecycle.create_time_sync_service()
    time_sync.set_simulation_step_handler(
        lambda _now, _duration: lifecycle.stop("done"), 1_000_000
    )
    del time_sync
    awaitable = lifecycle.start_lifecycle()
    del p
    gc.collect()
    assert ref() is not None
    assert asyncio.run(awaitable) == m.ParticipantState.Shutdown
    lifecycle = None
    del awaitable
    gc.collect()
    assert ref() is None


if __name__ == "__main__":
    scenario = sys.argv[1]
    if scenario.startswith("owner_"):
        check_owner(scenario.removeprefix("owner_"))
    elif scenario.startswith("cycle_"):
        check_cycle(scenario.removeprefix("cycle_"))
    elif scenario.startswith("cancel_"):
        check_cancellation(scenario.removeprefix("cancel_"))
    else:
        globals()["check_" + scenario]()
