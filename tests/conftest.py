from __future__ import annotations

# Register the shared fixture for every test module. Tests that need a running
# registry request it explicitly or use pytest.mark.usefixtures.
from common import silkit_registry as silkit_registry
