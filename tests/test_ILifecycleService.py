from __future__ import annotations

import SilKit_py as m
import asyncio
import threading
import subprocess, time
import pytest
from common import simpel_yamel, uri, silkit_registry


def test_callable_of_create_lifecycle_service():

    global simpel_yamel
    global uri
    cfg = m.participant_configuration_from_String(simpel_yamel)

    participant = m.create_participant(cfg, "TestParticipant", uri)

    assert callable(participant.create_lifecycle_service)

@pytest.mark.asyncio
async def test_create_lifecycle_service():
    global simpel_yamel
    global uri

    cfg = m.participant_configuration_from_String(simpel_yamel)
    participant = m.create_participant(cfg, "TestParticipant", uri)

    lifecycle_cfg = m.LifecycleConfiguration(m.OperationMode.Coordinated)

    # await the async constructor
    lifecycle_service = participant.create_lifecycle_service(lifecycle_cfg)

    assert lifecycle_service is not None
    assert isinstance(lifecycle_service, m.ILifecycleService)

@pytest.mark.asyncio
async def test_create_ITimeSyncService():
    global simpel_yamel
    global uri

    cfg = m.participant_configuration_from_String(simpel_yamel)
    participant = m.create_participant(cfg, "TestParticipant", uri)

    lifecycle_cfg = m.LifecycleConfiguration(m.OperationMode.Coordinated)

    # await the async constructor
    lifecycle_service = participant.create_lifecycle_service(lifecycle_cfg)

    time_service = lifecycle_service.create_time_sync_service()

    assert time_service is not None
    assert isinstance(time_service, m.ITimeSyncService)
    assert callable(time_service.set_simulation_step_handler)

@pytest.mark.asyncio
async def test_set_simulation_step_handler():
    global simpel_yamel
    global uri

    # Prepare participant
    cfg = m.participant_configuration_from_String(simpel_yamel)
    participant = m.create_participant(cfg, "TestParticipant", uri)

    # Coordinated lifecycle
    lifecycle_cfg = m.LifecycleConfiguration(m.OperationMode.Autonomous)
    lifecycle = participant.create_lifecycle_service(lifecycle_cfg)

    # Time sync service
    time_service = lifecycle.create_time_sync_service()

    # Track callback invocations
    calls = {}

    def step_handler(now, duration):
        # record values
        calls["now"] = now
        calls["duration"] = duration

        # stop lifecycle immediately after first tick
        lifecycle.stop("Test needs to stop")

    # Register handler with 1 ms step size
    time_service.set_simulation_step_handler(step_handler, int(1_000_000))

    # Start lifecycle – this returns an awaitable
    await lifecycle.start_lifecycle()

    # Lifecycle should now be finished
    assert "now" in calls
    assert "duration" in calls
    assert isinstance(calls["now"], int)
    assert isinstance(calls["duration"], int)
    assert calls["duration"] == 1_000_000

def test_lifecycle_state_callable():
    cfg = m.participant_configuration_from_String(simpel_yamel)
    participant = m.create_participant(cfg, "StateOnlyTest", uri)

    lifecycle_cfg = m.LifecycleConfiguration(m.OperationMode.Autonomous)
    lifecycle = participant.create_lifecycle_service(lifecycle_cfg)

    state = lifecycle.state()

    assert isinstance(state, m.ParticipantState)

@pytest.fixture
def system_controller(silkit_registry):
    run_sil_kit_monitor = False
    sil_kit_monitor = None
    # System Controller starten
    proc = subprocess.Popen([
        "./build-silkit/Release/sil-kit-system-controller.exe",
        "PubParticipant",
        "SubParticipant",
        "-u", uri,
        "-l", "Off"
    ],
    stdout=None,
    stderr=None)
    time.sleep(0.3)  # kurze Wartezeit
    if(run_sil_kit_monitor):
        sil_kit_monitor = subprocess.Popen([
            "./build-silkit/Release/sil-kit-monitor.exe",
            "-u", uri
        ],
        stdout=None,
        stderr=None)
        time.sleep(0.3)  # kurze Wartezeit
    yield
    proc.terminate()
    if(sil_kit_monitor):
        sil_kit_monitor.terminate()
    try:
        proc.wait(timeout=2)
        if(sil_kit_monitor):
            sil_kit_monitor.wait(timeout=2)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
        if(sil_kit_monitor):
            sil_kit_monitor.kill()
            sil_kit_monitor.wait()


@pytest.mark.asyncio
async def test_pub_sub_with_coordinated_lifecycle(system_controller):
    global simpel_yamel
    global uri

    # -------------------------------
    # Publisher Participant
    # -------------------------------
    cfg_pub = m.participant_configuration_from_String(simpel_yamel)
    pub = m.create_participant(cfg_pub, "PubParticipant", uri)

    pub_lc_cfg = m.LifecycleConfiguration(m.OperationMode.Coordinated)
    pub_lc = pub.create_lifecycle_service(pub_lc_cfg)
    pub_time = pub_lc.create_time_sync_service()

    pub_sub_spec = m.PubSubSpec("DataService", "text/plain")
    publisher = pub.create_data_publisher("PublisherController", pub_sub_spec)

    published = {"sent": False}

    received_event = asyncio.Event()

    def pub_step(now, duration):
        if not published["sent"]:
            publisher.publish(b"HelloSub")
            published["sent"] = True

    pub_time.set_simulation_step_handler(pub_step, int(1_000_000))

    # -------------------------------
    # Subscriber Participant
    # -------------------------------
    cfg_sub = m.participant_configuration_from_String(simpel_yamel)
    sub = m.create_participant(cfg_sub, "SubParticipant", uri)

    sub_lc_cfg = m.LifecycleConfiguration(m.OperationMode.Coordinated)
    sub_lc = sub.create_lifecycle_service(sub_lc_cfg)
    sub_time = sub_lc.create_time_sync_service()

    received = {}

    def on_data(subscriber, event):
        received["data"] = bytes(event.data)

        pub_lc.stop("Done")
        sub_lc.stop("Done")
        received_event.set()

    subscriber = sub.create_data_subscriber(
        "SubscriberController",
        pub_sub_spec,
        on_data
    )

    def sub_step(now, duration):
        pass

    sub_time.set_simulation_step_handler(sub_step, int(1_000_000))

    await asyncio.wait_for(
        asyncio.gather(
            pub_lc.start_lifecycle(),
            sub_lc.start_lifecycle(),
            received_event.wait(),
        ),
        timeout=1.0,
    )

    pub_lc.stop("Done")
    sub_lc.stop("Done")

    assert "data" in received
    assert received["data"] == b"HelloSub"


@pytest.fixture
def single_participant_controller(silkit_registry):
    proc = subprocess.Popen([
        "./build-silkit/Release/sil-kit-system-controller.exe",
        "AsyncNonBlockingTest",
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

import time
import threading
import asyncio
import pytest
import SilKit_py as m


@pytest.mark.asyncio
async def test_set_communication_ready_handler_async_and_complete(single_participant_controller):
    cfg = m.participant_configuration_from_String(simpel_yamel)
    participant = m.create_participant(cfg, "AsyncNonBlockingTest", uri)

    lifecycle_cfg = m.LifecycleConfiguration(m.OperationMode.Coordinated)
    lifecycle = participant.create_lifecycle_service(lifecycle_cfg)
    time_service = lifecycle.create_time_sync_service()

    loop = asyncio.get_running_loop()
    ready_called = asyncio.Event()
    step_called = asyncio.Event()

    seq = []
    seq_lock = threading.Lock()

    def comm_ready_handler():
        with seq_lock:
            seq.append("handler")

        # signal "handler called" to the test
        loop.call_soon_threadsafe(ready_called.set)

        # simulate async work, then complete
        def complete_later():
            time.sleep(0.05)
            with seq_lock:
                seq.append("complete")
            lifecycle.complete_communication_ready_handler_async()

        threading.Thread(target=complete_later, daemon=True).start()

    lifecycle.set_communication_ready_handler_async(comm_ready_handler)

    def step_handler(now, duration):
        with seq_lock:
            seq.append("step")
        loop.call_soon_threadsafe(step_called.set)
        lifecycle.stop("done")

    time_service.set_simulation_step_handler(step_handler, int(1_000_000))

    await asyncio.wait_for(
        asyncio.gather(
            lifecycle.start_lifecycle(),
            ready_called.wait(),
            step_called.wait(),
        ),
        timeout=3.0,
    )

    with seq_lock:
        assert "handler" in seq
        assert "complete" in seq
        assert "step" in seq
        # step must not happen before completion
        assert seq.index("complete") < seq.index("step")