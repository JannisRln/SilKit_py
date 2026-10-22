# Changelog

### 1.0.5 
 
- 🐛 Add Python 3.13 and 3.14 wheel support

### 1.0.4

- Added Windows and Linux build support, including native library packaging and cross-platform development tasks.
- Fixed CAN DLC calculation, payload validation, timestamps, and public CAN/RPC exports.
- Added a PyPI source-release workflow with Linux/Windows installation checks and Trusted Publishing; removed unused platform workflows.

### 1.0.3

- Added support for the RPC API.

### 1.0.2

- Added support for `ICanController`.
- Split tests into separate files.

### 1.0.1

- Added `set_communication_ready_handler_async`.
- Split C++ bindings into multiple files.

### 1.0.0

- Added initial Python bindings for participant configuration, lifecycle, time synchronization, and Pub/Sub.
- Added a simple publish/subscribe example.
