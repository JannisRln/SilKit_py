# Auto-generated type stub for SilKit Python Wrapper

from typing import Optional, Awaitable, Callable, List, overload
from enum import Enum

# Version attribute
__version__: str

class ParticipantState(Enum):
    Invalid = 0 # An invalid participant state 
    ServicesCreated = 10 # The controllers created state 
    CommunicationInitializing = 20 # The communication initializing state 
    CommunicationInitialized = 30 # The communication initialized state 
    ReadyToRun = 40 # The initialized state 
    Running = 50 # The running state 
    Paused = 60 # The paused state 
    Stopping = 70 # The stopping state 
    Stopped = 80 # The stopped state 
    Error = 90 # The error state 
    ShuttingDown = 100 # The shutting down state 
    Shutdown = 110 # The shutdown state 
    Aborting = 120 # The aborting state 

class IParticipantConfiguration:
    """SilKit participant configuration."""

def participant_configuration_from_String(json_string: str) -> IParticipantConfiguration:
    """Create a participant configuration from a JSON string."""
    ...

def create_participant(
    participant_config: IParticipantConfiguration,
    participant_name: str,
    registry_uri: str = "silkit://localhost:8500",
) -> IParticipant:
    """Create and return a SilKit participant."""
    ...


DataMessageHandler = Callable[[IDataSubscriber, DataMessageEvent], None]

class IParticipant:
    """SilKit participant"""
    def create_lifecycle_service(self, lifecycle_service_config: LifecycleConfiguration) -> ILifecycleService:
        """Create a SilKit::Services::Orchestration::ILifecycleService"""
        ...
    def create_data_publisher(
        self,
        canonical_name: str,
        data_spec: PubSubSpec,
        history: int = 0,
    ) -> IDataPublisher: ...
    def create_data_subscriber(
        self,
        canonical_name: str,
        data_spec: PubSubSpec,
        data_message_handler: DataMessageHandler,
    ) -> IDataSubscriber: ...
    def get_logger(self)-> ILogger:
        ...

class ILifecycleService:
    """SilKit::Services::Orchestration::ILifecycleService"""
    def start_lifecycle(self) -> Awaitable[ParticipantState]:
        ...
    def stop(self, reason:str):
        ...
    def create_time_sync_service(self) ->ITimeSyncService:
        ...

class OperationMode(Enum):
    Invalid = 0 #SilKit_OperationMode_Invalid
    Coordinated = 10 #SilKit_OperationMode_Coordinated
    Autonomous = 20 #SilKit_OperationMode_Autonomous
class LifecycleConfiguration:
    def __init__(self, operation_mode: Optional[OperationMode] = ...) -> None: ...
    operation_mode: OperationMode

class ITimeSyncService:
    def set_simulation_step_handler(
        self,
        handler: Callable[[int, int], None],
        initial_step_size: int,
    ) -> None:
        """
        Register a simulation step callback.

        Parameters
        ----------
        handler : Callable[[int, int], None]
            A function called for each simulation step.
            Arguments are:
                now_ns: int        # current simulation time in nanoseconds
                duration_ns: int   # step duration in nanoseconds

        initial_step_size : int
            Initial step size in nanoseconds.
        """
        ...

class MatchingLabel:
    class Kind(Enum):
        Optional = 0
        Required = 1

    key: str
    value: str
    kind: Kind

class IDataPublisher:
    # Methods not shown; add as needed
    def publish(self, data: bytes) -> None: ...
class IDataSubscriber:
    # Methods not shown; add as needed
    ...

class DataMessageEvent:
    @property
    def timestamp(self) -> int: ...
    @property
    def data(self) -> bytes: ...

class PubSubSpec:
    def __init__(self, topic: str = "", mediaType: str = "") -> None: ...
    
    @overload
    def add_label(self, label: MatchingLabel) -> None: ...
    @overload
    def add_label(self, key: str, value: str, kind: MatchingLabel.Kind) -> None: ...

    @property
    def topic(self) -> str: ...
    @property
    def media_type(self) -> str: ...
    @property
    def labels(self) -> List[MatchingLabel]: ...

class LogLevel(Enum):
    Trace = 0 # Detailed debug-level messages
    Debug = 1 # Normal debug-level messages
    Info = 2 # Informational content
    Warn = 3 # Warnings
    Error = 4 # Non-critical errors
    Critical = 5 # Critical errors
    Off = 0xffffffff # Logging is disabled

class ILogger:
    """SilKit::Services::Logging::ILogge"""
    def log(self, level:LogLevel, msg:str)-> None:
        ...
