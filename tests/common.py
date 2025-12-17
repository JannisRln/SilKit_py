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