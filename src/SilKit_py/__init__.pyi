# Auto-generated type stub for SilKit Python Wrapper

from typing import Optional, Awaitable, Callable, List, overload, TypeAlias, Any
from enum import Enum
import enum
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
    def create_can_controller(self, canonical_name:str, network_name:str) -> ICanController:
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

CommunicationReadyHandler = Callable[[],None]

class ILifecycleService:
    """SilKit::Services::Orchestration::ILifecycleService"""
    def set_communication_ready_handler(self, communication_ready_handler: CommunicationReadyHandler ) -> None: ...
    def set_communication_ready_handler_async(self, communication_ready_handler: CommunicationReadyHandler ) -> None: ...
    def complete_communication_ready_handler_async(self) -> None: ...
    def start_lifecycle(self) -> Awaitable[ParticipantState]:
        ...
    def stop(self, reason:str):
        ...
    def state(self)-> ParticipantState: ...
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



HandlerId: TypeAlias = int
DirectionMask: TypeAlias = int
CanFrameFlagMask: TypeAlias = int
CanTransmitStatusMask: TypeAlias = int


class CanFrame:
    can_id: int          # uint32_t canId :contentReference[oaicite:0]{index=0}
    flags: int           # CanFrameFlagMask flags :contentReference[oaicite:1]{index=1}
    dlc: int             # uint16_t dlc :contentReference[oaicite:2]{index=2}
    sdt: int             # uint8_t sdt :contentReference[oaicite:3]{index=3}
    vcid: int            # uint8_t vcid :contentReference[oaicite:4]{index=4}
    af: int              # uint32_t af :contentReference[oaicite:5]{index=5}

    def __init__(self) -> None: ...
    @property
    def data(self) -> bytes: ...
    @data.setter
    def data(self, b: bytes) -> None: ...

class TransmitDirection(enum.IntFlag):
    Undefined: TransmitDirection
    RX: TransmitDirection
    TX: TransmitDirection
    TXRX: TransmitDirection
# enum class SilKit::Services::TransmitDirection : uint8_t :contentReference[oaicite:6]{index=6}


class CanControllerState(enum.IntEnum):
    Uninit: CanControllerState
    Stopped: CanControllerState
    Started: CanControllerState
    Sleep: CanControllerState
# :contentReference[oaicite:7]{index=7}


class CanErrorState(enum.IntEnum):
    NotAvailable: CanErrorState
    ErrorActive: CanErrorState
    ErrorPassive: CanErrorState
    BusOff: CanErrorState
# :contentReference[oaicite:8]{index=8}


class CanFrameFlag(enum.IntFlag):
    Ide: CanFrameFlag
    Rtr: CanFrameFlag
    Fdf: CanFrameFlag
    Brs: CanFrameFlag
    Esi: CanFrameFlag
    Xlf: CanFrameFlag
    Sec: CanFrameFlag
# :contentReference[oaicite:9]{index=9}


class CanTransmitStatus(enum.IntFlag):
    Transmitted: CanTransmitStatus
    Canceled: CanTransmitStatus
    TransmitQueueFull: CanTransmitStatus
# :contentReference[oaicite:10]{index=10}


CanTransmitStatus_DefaultMask: int


class CanFrameEvent:
    timestamp: int          # std::chrono::nanoseconds :contentReference[oaicite:11]{index=11}
    frame: CanFrame         # :contentReference[oaicite:12]{index=12}
    direction: TransmitDirection  # :contentReference[oaicite:13]{index=13}
    userContext: int        # void* :contentReference[oaicite:14]{index=14}


class ICanController:
    def set_baud_rate(self, rate: int, fd_rate: int, xl_rate: int) -> None: ...
    def reset(self) -> None: ...
    def start(self) -> None: ...
    def stop(self) -> None: ...
    def sleep(self) -> None: ...

    def send_frame(
        self,
        can_id: int,
        data: bytes,
        flags: int = 0,
        dlc: Optional[int] = ...,
        sdt: int = 0,
        vcid: int = 0,
        af: int = 0,
        user_context: int = 0,
    ) -> None: ...

    def add_frame_handler(
        self,
        handler: Callable[[ICanController, CanFrameEvent], Any],
        direction_mask: DirectionMask = ...,
    ) -> HandlerId: ...
# CAN controller concepts and data structures :contentReference[oaicite:15]{index=15}
