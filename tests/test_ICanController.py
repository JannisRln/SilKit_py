import asyncio
import subprocess
import time
import threading
import pytest

import SilKit_py as m

from common import simpel_yamel, uri, silkit_registry

@pytest.fixture
def can_env(silkit_registry, request):
    # unique names per test to avoid collisions
    pub_name = f"CanPub_{request.node.name}"
    sub_name = f"CanSub_{request.node.name}"

    proc = subprocess.Popen([
        "./build-silkit/Release/sil-kit-system-controller.exe",
        pub_name,
        sub_name,
        "-u", uri,
        "-l", "Off",
    ])
    time.sleep(0.3)

    cfg_pub = m.participant_configuration_from_String(simpel_yamel)
    cfg_sub = m.participant_configuration_from_String(simpel_yamel)

    pub = m.create_participant(cfg_pub, pub_name, uri)
    sub = m.create_participant(cfg_sub, sub_name, uri)

    pub_lc = pub.create_lifecycle_service(m.LifecycleConfiguration(m.OperationMode.Coordinated))
    sub_lc = sub.create_lifecycle_service(m.LifecycleConfiguration(m.OperationMode.Coordinated))

    pub_ts = pub_lc.create_time_sync_service()
    sub_ts = sub_lc.create_time_sync_service()

    pub_can = pub.create_can_controller("PubCanCtrl", "CAN1")
    sub_can = sub.create_can_controller("SubCanCtrl", "CAN1")

    # configure rates
    pub_can.set_baud_rate(500_000, 2_000_000, 0)
    sub_can.set_baud_rate(500_000, 2_000_000, 0)

    env = {
        "pub": pub, "sub": sub,
        "pub_lc": pub_lc, "sub_lc": sub_lc,
        "pub_ts": pub_ts, "sub_ts": sub_ts,
        "pub_can": pub_can, "sub_can": sub_can,
        "pub_name": pub_name, "sub_name": sub_name,
    }

    yield env

    # best-effort cleanup
    try:
        pub_lc.stop("cleanup")
    except Exception:
        pass
    try:
        sub_lc.stop("cleanup")
    except Exception:
        pass

    proc.terminate()
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()





def test_can_controller_created(can_env):
    assert isinstance(can_env["pub_can"], m.ICanController)
    assert isinstance(can_env["sub_can"], m.ICanController)

    # methods exist (smoke)
    assert callable(can_env["pub_can"].set_baud_rate)
    assert callable(can_env["pub_can"].send_frame)
    assert callable(can_env["pub_can"].add_frame_handler)


@pytest.mark.asyncio
async def test_can_frame_receive_handler(can_env):
    loop = asyncio.get_running_loop()
    rx_event = asyncio.Event()
    received = {}

    def on_frame(ctrl, evt):
        print("on_frame")
        received["can_id"] = evt.frame.can_id
        received["data"] = bytes(evt.frame.data)
        loop.call_soon_threadsafe(rx_event.set)
        can_env["pub_lc"].stop("done")
        can_env["sub_lc"].stop("done")

    rx_id = can_env["sub_can"].add_frame_handler(on_frame)  # default RX

    sent = {"v": False}

    def pub_step(now, duration):
        if sent["v"]:
            return

        can_env["pub_can"].send_frame(
            0x123,
            b"\x01\x02\x03\x04",
            0,      # flags
            None,   # dlc
            0, 0, 0,
            0,      # user_context
        )
        sent["v"] = True

    def _noop_step(now, duration):
        pass

    def pub_start():
        can_env["pub_can"].start()

    def sub_start():
        can_env["sub_can"].start()

    can_env["pub_lc"].set_communication_ready_handler(pub_start)
    can_env["sub_lc"].set_communication_ready_handler(sub_start)

    can_env["pub_ts"].set_simulation_step_handler(pub_step, int(1_000_000))
    can_env["sub_ts"].set_simulation_step_handler(_noop_step, int(1_000_000))


    await asyncio.wait_for(
        asyncio.gather(
            can_env["pub_lc"].start_lifecycle(),
            can_env["sub_lc"].start_lifecycle(),
            rx_event.wait(),
        ),
        timeout=3.0,
    )

    assert received["can_id"] == 0x123
    assert received["data"] == b"\x01\x02\x03\x04"