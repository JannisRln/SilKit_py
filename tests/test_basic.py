from __future__ import annotations

import SilKit_py as m
import asyncio

simpel_yamel = \
"""
Logging:
  Sinks:
  - Type: Stdout
    Level: Off
    LogName: SimpleParticipantLog
    HealthCheck:
      SoftResponseTimeout: 500
      HardResponseTimeout: 1000
"""

uri= "silkit://localhost:8501"

def test_version():
    assert m.__version__ == "1.0.0"

import subprocess, time
import pytest

@pytest.fixture(scope="session", autouse=True)
def silkit_registry():

    global uri

    proc = subprocess.Popen([
        "./build-silkit/Release/sil-kit-registry.exe",
        "--listen-uri", uri,
        "-l", "Off"
    ],
    stdout=None,
    stderr=None)
    time.sleep(0.5)  # wait for registry to start
    yield
    proc.terminate()
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()

def test_participant_configuration_from_String_is_callable():
    assert callable(m.participant_configuration_from_String)

def test_create_participant_configuration():

    global simpel_yamel

    cfg = m.participant_configuration_from_String(simpel_yamel)

    assert cfg is not None
    assert isinstance(cfg, m.IParticipantConfiguration)


def test_create_participant():
    assert callable(m.create_participant)

def test_create_participant_with_cfg():
    
    global simpel_yamel
    global uri

    cfg = m.participant_configuration_from_String(simpel_yamel)

    simpel_participant = m.create_participant(cfg, "TestParticipant", uri)

    assert simpel_participant is not None
    assert isinstance(simpel_participant, m.IParticipant)

def test_get_logger_from_participant():
    global simpel_yamel
    global uri

    cfg = m.participant_configuration_from_String(simpel_yamel)

    simpel_participant = m.create_participant(cfg, "TestParticipant", uri)

    logger = simpel_participant.get_logger()

    assert logger is not None
    assert isinstance(logger, m.ILogger)
    assert callable(logger.log)

def test_log_with_enum_value():
    global simpel_yamel
    global uri

    cfg = m.participant_configuration_from_String(simpel_yamel)

    simpel_participant = m.create_participant(cfg, "TestParticipant", uri)

    logger = simpel_participant.get_logger()

    logger.log(m.LogLevel.Critical, "Test")


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


def test_create_data_publisher():
    global simpel_yamel
    global uri

    # Prepare participant
    cfg = m.participant_configuration_from_String(simpel_yamel)
    participant = m.create_participant(cfg, "TestParticipant", uri)

    pub_sub_spec = m.PubSubSpec("DataService", "text/plain")

    assert isinstance( pub_sub_spec, m.PubSubSpec)

    data_publisher = participant.create_data_publisher("PublisherController", pub_sub_spec)

    assert data_publisher is not None
    assert isinstance(data_publisher, m.IDataPublisher)

def test_create_data_subscriber():
    global simpel_yamel
    global uri

    # Prepare participant
    cfg = m.participant_configuration_from_String(simpel_yamel)
    participant = m.create_participant(cfg, "TestParticipant", uri)

    pub_sub_spec = m.PubSubSpec("DataService", "text/plain")

    assert isinstance( pub_sub_spec, m.PubSubSpec)

    def handler(subscriber: m.IDataSubscriber, event: m.DataMessageEvent) -> None:
        ...

    data_subscriber = participant.create_data_subscriber("SubscriberController", pub_sub_spec, handler)

    assert data_subscriber is not None
    assert isinstance(data_subscriber, m.IDataSubscriber)

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
