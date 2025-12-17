from __future__ import annotations

import SilKit_py as m
import asyncio
import threading
import subprocess, time
import pytest

from common import simpel_yamel, uri

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