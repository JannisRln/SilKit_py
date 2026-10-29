Troubleshooting
===============

Registry connection fails
-------------------------

Confirm that the registry is running and its URI matches the participant.
The README example connects to port 8501.

Coordinated simulation hangs
----------------------------

Ensure that the system controller is active and that all expected
participants are connected. A registry alone is insufficient.

Installation fails
------------------

Check that you are using CPython 3.12-3.14 and that a compatible wheel
exists for your OS and architecture. Source builds require CMake,
a C++ toolchain, and the SIL Kit submodule.

CAN or Pub/Sub data is unexpected
--------------------------------

Check that network names, topics, and payload encodings match the peer.
CAN payloads are not padded automatically.
