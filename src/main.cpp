#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <iostream>

#include "silkit/SilKit.hpp"

#include "ICanController.hpp"
#include "ILifecycleService.hpp"
#include "PubSub.hpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
    m.doc() = "SilKit Python Wrapper";

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif

    py::enum_<SilKit::Services::Orchestration::ParticipantState>(m, "ParticipantState")
        .value("Invalid",                         SilKit::Services::Orchestration::ParticipantState::Invalid)
        .value("ServicesCreated",                 SilKit::Services::Orchestration::ParticipantState::ServicesCreated)
        .value("CommunicationInitializing",       SilKit::Services::Orchestration::ParticipantState::CommunicationInitializing)
        .value("CommunicationInitialized",        SilKit::Services::Orchestration::ParticipantState::CommunicationInitialized)
        .value("ReadyToRun",                      SilKit::Services::Orchestration::ParticipantState::ReadyToRun)
        .value("Running",                         SilKit::Services::Orchestration::ParticipantState::Running)
        .value("Paused",                          SilKit::Services::Orchestration::ParticipantState::Paused)
        .value("Stopping",                        SilKit::Services::Orchestration::ParticipantState::Stopping)
        .value("Stopped",                         SilKit::Services::Orchestration::ParticipantState::Stopped)
        .value("Error",                           SilKit::Services::Orchestration::ParticipantState::Error)
        .value("ShuttingDown",                    SilKit::Services::Orchestration::ParticipantState::ShuttingDown)
        .value("Shutdown",                        SilKit::Services::Orchestration::ParticipantState::Shutdown)
        .value("Aborting",                        SilKit::Services::Orchestration::ParticipantState::Aborting);

    py::class_<SilKit::Config::IParticipantConfiguration, py::smart_holder>(m, "IParticipantConfiguration");

    m.def("participant_configuration_from_String",
          &SilKit::Config::ParticipantConfigurationFromString,
          py::arg("string"));

    m.def(
        "create_participant",
        py::overload_cast<
            std::shared_ptr<SilKit::Config::IParticipantConfiguration>,
            const std::string&,
            const std::string&
        >(&SilKit::CreateParticipant),
        py::arg("participant_config"),
        py::arg("participant_name"),
        py::arg("registry_uri") = "silkit://localhost:8500");

    py::class_<SilKit::IParticipant, py::smart_holder>(m, "IParticipant")
        .def("create_lifecycle_service",
             &SilKit::IParticipant::CreateLifecycleService,
            py::arg("lifecycle_service_config"),
            py::return_value_policy::reference)
        .def("create_can_controller",
            &SilKit::IParticipant::CreateCanController,
            py::arg("canonical_name"),
            py::arg("network_name"),
            py::return_value_policy::reference)
        .def("create_data_publisher",
            &SilKit::IParticipant::CreateDataPublisher,
            py::arg("canonical_name"),
            py::arg("data_spec"),
            py::arg("history") = 0,
            py::return_value_policy::reference)
        .def("create_data_subscriber",
            [](SilKit::IParticipant* self,
            const std::string& canonicalName,
            const SilKit::Services::PubSub::PubSubSpec& dataSpec,
            py::function pyHandler)
            {
                SilKit::Services::PubSub::DataMessageHandler handler =
                    [pyHandler](SilKit::Services::PubSub::IDataSubscriber* sub,
                                const SilKit::Services::PubSub::DataMessageEvent& evt)
                    {
                        py::gil_scoped_acquire gil;

                        pyHandler(sub, evt);
                    };

                auto* subscriber =
                    self->CreateDataSubscriber(canonicalName, dataSpec, std::move(handler));

                return subscriber;
            },
            py::arg("canonical_name"),
            py::arg("data_spec"),
            py::arg("data_message_handler"),
            py::return_value_policy::reference,
            py::keep_alive<0, 3>()
        )
        .def("get_logger", 
             &SilKit::IParticipant::GetLogger,
             py::return_value_policy::reference);

    py::enum_<SilKit::Services::Logging::Level>(m, "LogLevel")
        .value("Trace", SilKit::Services::Logging::Level::Trace) 
        .value("Debug", SilKit::Services::Logging::Level::Debug) 
        .value("Info", SilKit::Services::Logging::Level::Info) 
        .value("Warn", SilKit::Services::Logging::Level::Warn) 
        .value("Error", SilKit::Services::Logging::Level::Error) 
        .value("Critical", SilKit::Services::Logging::Level::Critical) 
        .value("Off", SilKit::Services::Logging::Level::Off);

    py::class_<SilKit::Services::Logging::ILogger, py::smart_holder>(m, "ILogger")
        .def("log", 
            &SilKit::Services::Logging::ILogger::Log,
            py::arg("level"),
            py::arg("msg"));

    bind_ICanController( m );

    bind_ILifecycleService( m );

    bind_PubSub( m );

}
