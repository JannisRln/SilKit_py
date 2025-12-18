from __future__ import annotations

import SilKit_py as m
import asyncio
import threading
import subprocess, time
import pytest

from common import simpel_yamel, uri

def test_version():
    assert m.__version__ == "1.0.3"

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






