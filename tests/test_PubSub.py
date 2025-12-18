from __future__ import annotations

import SilKit_py as m
import asyncio
import threading
import subprocess, time
import pytest

from common import simpel_yamel, uri, silkit_registry


def _create_participant(name: str):
    cfg = m.participant_configuration_from_String(simpel_yamel)
    return m.create_participant(cfg, name, uri)

def test_create_data_publisher():
    global simpel_yamel
    global uri

    participant = _create_participant( "TestParticipant" )

    pub_sub_spec = m.PubSubSpec("DataService", "text/plain")

    assert isinstance( pub_sub_spec, m.PubSubSpec)

    data_publisher = participant.create_data_publisher("PublisherController", pub_sub_spec)

    assert data_publisher is not None
    assert isinstance(data_publisher, m.IDataPublisher)

def test_create_data_subscriber():
    global simpel_yamel
    global uri

    participant = _create_participant( "TestParticipant" )

    pub_sub_spec = m.PubSubSpec("DataService", "text/plain")

    assert isinstance( pub_sub_spec, m.PubSubSpec)

    def handler(subscriber: m.IDataSubscriber, event: m.DataMessageEvent) -> None:
        ...

    data_subscriber = participant.create_data_subscriber("SubscriberController", pub_sub_spec, handler)

    assert data_subscriber is not None
    assert isinstance(data_subscriber, m.IDataSubscriber)

# ---------------------------------------------------------------------------
# RpcCallStatus enum
# ---------------------------------------------------------------------------

def test_rpc_call_status_enum_values():
    assert m.RpcCallStatus.Success.name == "Success"
    assert m.RpcCallStatus.ServerNotReachable.name == "ServerNotReachable"
    assert m.RpcCallStatus.UndefinedError.name == "UndefinedError"
    assert m.RpcCallStatus.InternalServerError.name == "InternalServerError"
    assert m.RpcCallStatus.Timeout.name == "Timeout"

# ---------------------------------------------------------------------------
# RpcSpec
# ---------------------------------------------------------------------------

def test_rpc_spec_default_ctor():
    spec = m.RpcSpec()
    assert isinstance(spec, m.RpcSpec)


def test_rpc_spec_ctor_with_args_and_properties():
    spec = m.RpcSpec("MyFunction", m.media_type_rpc())

    assert spec.function_name == "MyFunction"
    assert spec.media_type == m.media_type_rpc()
    assert isinstance(spec.labels, list)


def test_rpc_spec_add_label_variants():
    spec = m.RpcSpec("Fn", "mt")

    # key/value/kind overload
    spec.add_label("key1", "value1", m.MatchingLabelKind.Optional)

    # MatchingLabel overload
    lbl = m.MatchingLabel("key2", "value2", m.MatchingLabelKind.Mandatory)
    spec.add_label(lbl)

    labels = spec.labels
    assert len(labels) == 2

# ---------------------------------------------------------------------------
# RpcCallEvent / RpcCallResultEvent (read-only API surface)
# ---------------------------------------------------------------------------

def test_rpc_call_event_properties_exist():
    # Cannot instantiate directly; check attributes on the type
    assert hasattr(m.RpcCallEvent, "timestamp_ns")
    assert hasattr(m.RpcCallEvent, "call_handle")
    assert hasattr(m.RpcCallEvent, "argument_data")


def test_rpc_call_result_event_properties_exist():
    assert hasattr(m.RpcCallResultEvent, "timestamp_ns")
    assert hasattr(m.RpcCallResultEvent, "user_context")
    assert hasattr(m.RpcCallResultEvent, "call_status")
    assert hasattr(m.RpcCallResultEvent, "result_data")

# ---------------------------------------------------------------------------
# IRpcClient / IRpcServer creation and callability
# ---------------------------------------------------------------------------

def test_create_rpc_client_and_server_callable():
    participant = _create_participant("RpcTestParticipant")

    spec = m.RpcSpec("TestRpc", "application/octet-stream")

    def passClient(cli: m.IRpcClient, event: m.RpcCallResultEvent):
        pass
    def passServer(cli: m.IRpcServer, event: m.RpcCallEvent):
        pass

    client = participant.create_rpc_client("RpcClient", spec, passClient)
    server = participant.create_rpc_server("RpcServer", spec, passServer)

    assert client is not None
    assert server is not None

    assert isinstance(client, m.IRpcClient)
    assert isinstance(server, m.IRpcServer)

    assert callable(client.call)
    assert callable(client.call_with_timeout)
    assert callable(client.set_call_result_handler)

    assert callable(server.submit_result)
    assert callable(server.set_call_handler)


def test_set_rpc_handlers():
    participant = _create_participant("RpcHandlerTest")

    spec = m.RpcSpec("TestRpc", m.media_type_rpc())

    def on_call_result(cli: m.IRpcClient, event: m.RpcCallResultEvent):
        assert isinstance(event.call_status, m.RpcCallStatus)
        client_called["ok"] = True

    def on_call(srv: m.IRpcServer, event: m.RpcCallEvent):
        assert isinstance(event.call_handle, int)
        assert isinstance(event.argument_data, (bytes, bytearray))
        server_called["ok"] = True

    client = participant.create_rpc_client("RpcClient", spec, on_call_result)
    server = participant.create_rpc_server("RpcServer", spec, on_call)

    client_called = {"ok": False}
    server_called = {"ok": False}

    # No actual call executed here – this test validates binding + registration only
    assert client_called["ok"] is False
    assert server_called["ok"] is False


def test_rpc_client_call_argument_types():
    participant = _create_participant("RpcCallTypeTest")

    spec = m.RpcSpec("TestRpc", m.media_type_rpc())

    client_called = {"ok": False}

    def on_call_result(cli: m.IRpcClient, event: m.RpcCallResultEvent):
        assert isinstance(event.call_status, m.RpcCallStatus)
        client_called["ok"] = True

    client = participant.create_rpc_client("RpcClient", spec, on_call_result)

    # user_context as None
    client.call(b"payload", None)

    # user_context as integer pointer
    client.call(b"payload", 123456)

    # timeout variant
    client.call_with_timeout(b"payload", 1_000_000, None)


@pytest.fixture
def Rpc_test_controller(silkit_registry):
    proc = subprocess.Popen([
        "./build-silkit/Release/sil-kit-system-controller.exe",
        "RpcClient",
        "RpcServer",
        "-u", uri,
        "-l", "Off"
    ])

    time.sleep(0.3)
    yield

    proc.terminate()
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()

@pytest.mark.asyncio
async def test_call_rpc_handlers(Rpc_test_controller):
    participant_client = _create_participant("RpcClient")
    participant_server = _create_participant("RpcServer")

    spec = m.RpcSpec("TestRpc", m.media_type_rpc())

    client_called = {"ok": False}
    server_called = {"ok": False}

    loop = asyncio.get_running_loop()
    rpc_client_called_back = asyncio.Event()
    rpc_server_called = asyncio.Event()

    lifecycle_cfg = m.LifecycleConfiguration(m.OperationMode.Coordinated)
    lifecycle_client = participant_client.create_lifecycle_service(lifecycle_cfg)
    lifecycle_server = participant_server.create_lifecycle_service(lifecycle_cfg)



    time_service_client = lifecycle_client.create_time_sync_service()
    time_service_server = lifecycle_server.create_time_sync_service()



    def on_call_result(cli: m.IRpcClient, event: m.RpcCallResultEvent):
        print("py on_call_result")
        assert isinstance(event.call_status, m.RpcCallStatus)
        client_called["ok"] = True
        loop.call_soon_threadsafe(rpc_client_called_back.set)
        lifecycle_client.stop("done")
        lifecycle_server.stop("done")

    def on_call(srv: m.IRpcServer, event: m.RpcCallEvent):
        print("py on_call")
        assert isinstance(event.call_handle, int)
        assert isinstance(event.argument_data, (bytes, bytearray))
        server_called["ok"] = True
        srv.submit_result(event.call_handle, b"response")
        loop.call_soon_threadsafe(rpc_server_called.set)

    client = participant_client.create_rpc_client("RpcClient", spec, on_call_result)
    server = participant_server.create_rpc_server("RpcServer", spec, on_call)

    sent = {"v": False}

    def step_handler_client(now, duration):
        if sent["v"]:
            return
        print("py client.call")
        client.call(b"test", None)
        sent["v"] = True

    def step_handler_server(now, duration):
        pass

    time_service_client.set_simulation_step_handler(step_handler_client, int(1_000_000))
    time_service_server.set_simulation_step_handler(step_handler_server, int(1_000_000))
    
    await asyncio.wait_for(
        asyncio.gather(
            lifecycle_client.start_lifecycle(),
            lifecycle_server.start_lifecycle(),
        ),
        timeout=3.0,
    )

    assert server_called["ok"] is True
    assert client_called["ok"] is True


