Core concepts
=============

Participants
------------

A participant connects to the registry with a unique name, configuration,
and registry URI.

Lifecycle
---------

Autonomous lifecycle mode runs independently; coordinated mode requires a
system controller and the configured participants. The registry itself does
not manage coordinated simulation startup.

Simulation time
---------------

The time synchronization service invokes synchronous step callbacks.
Step durations and timestamps in the binding examples are in nanoseconds.
The lifecycle startup method can be awaited from asyncio, but this does
not make the simulation-step callback asynchronous.

Communication
-------------

SIL Kit participants communicate through matching topics and payload
encodings for Pub/Sub, matching RPC endpoints, or CAN networks.
The binding implements a subset of SIL Kit services.
