from __future__ import annotations

import SilKit_py as silkit


def test_all_public_exports_exist():
    namespace = {}
    exec("from SilKit_py import *", namespace)
    assert set(silkit.__all__) <= namespace.keys()
    for name in silkit.__all__:
        assert namespace[name] is getattr(silkit, name)
    assert "ParticipantState" in namespace
    assert "IParticipantConfiguration" in namespace
    assert "RpcSpec" in namespace
    assert "IRpcClient" in namespace
    assert "CanFrameEvent" in namespace
    assert "CanTransmitStatus_DefaultMask" in namespace
