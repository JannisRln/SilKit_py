"""Cancellable lifecycle waiting without occupying asyncio's thread pool."""

from __future__ import annotations

import asyncio


async def wait_for_completion(completion):
    # The completion owns the lifecycle/participant while this coroutine waits.
    # Cancellation drops only this wait; callers can still stop the simulation
    # explicitly. No executor worker remains blocked during asyncio.run shutdown.
    while not completion.ready():
        await asyncio.sleep(0.001)
    return completion.result()
