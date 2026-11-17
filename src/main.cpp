#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

#include "silkit/SilKit.hpp"

#include "ICanController.hpp"
#include "ILifecycleService.hpp"
#include "PubSub.hpp"
#include "PythonCallback.hpp"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
    m.doc() = "SilKit Python Wrapper";
    py::class_<SilKitPy::CallbackState>(m, "_CallbackState", py::dynamic_attr());
    SilKitPy::RegisterShutdownHook();

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
        [](std::shared_ptr<SilKit::Config::IParticipantConfiguration> config,
           const std::string& name, const std::string& uri) {
            auto participant = SilKit::CreateParticipant(std::move(config), name, uri);
            return std::unique_ptr<SilKit::IParticipant, SilKitPy::ParticipantDeleter>(participant.release());
        },
        py::arg("participant_config"),
        py::arg("participant_name"),
        py::arg("registry_uri") = "silkit://localhost:8500",
        py::call_guard<py::gil_scoped_release>());

    // Destruction joins native threads, which may still be running callbacks.
    py::class_<SilKit::IParticipant, py::smart_holder>(
        m, "IParticipant", py::dynamic_attr(), py::release_gil_before_calling_cpp_dtor())
        .def("create_lifecycle_service",
            [](py::object self, SilKit::Services::Orchestration::LifecycleConfiguration config) {
                auto* participant = self.cast<SilKit::IParticipant*>();
                SilKit::Services::Orchestration::ILifecycleService* service;
                { py::gil_scoped_release release; service = participant->CreateLifecycleService(config); }
                SilKitPy::RegisterLifecycle(participant, service);
                return SilKitPy::BorrowWithOwner(service, self);
            }, py::arg("lifecycle_service_config"))
        .def("create_can_controller",
            [](py::object self, const std::string& name, const std::string& network) {
                auto* participant = self.cast<SilKit::IParticipant*>();
                SilKit::Services::Can::ICanController* controller;
                { py::gil_scoped_release release; controller = participant->CreateCanController(name, network); }
                return SilKitPy::BorrowWithOwner(controller, self);
            }, py::arg("canonical_name"), py::arg("network_name"))
        .def("create_data_publisher",
            [](py::object self, const std::string& name, SilKit::Services::PubSub::PubSubSpec spec, size_t history) {
                auto* participant = self.cast<SilKit::IParticipant*>();
                SilKit::Services::PubSub::IDataPublisher* publisher;
                { py::gil_scoped_release release; publisher = participant->CreateDataPublisher(name, spec, history); }
                return SilKitPy::BorrowWithOwner(publisher, self);
            },
            py::arg("canonical_name"),
            py::arg("data_spec"),
            py::arg("history") = 0)
        .def("create_data_subscriber",
            [](py::object self,
            const std::string& canonicalName,
            SilKit::Services::PubSub::PubSubSpec dataSpec,
            py::function pyHandler)
            {
                auto* participant = self.cast<SilKit::IParticipant*>();
                SilKitPy::CallbackRegistration registration(self, std::move(pyHandler));
                SilKit::Services::PubSub::IDataSubscriber* subscriber;
                {
                    auto handler = registration.callback();
                    py::gil_scoped_release release;
                    subscriber = participant->CreateDataSubscriber(canonicalName, dataSpec, std::move(handler));
                }
                registration.commit(SilKitPy::CallbackKey(subscriber, "data"));
                return SilKitPy::BorrowWithOwner(subscriber, self);
            },
            py::arg("canonical_name"),
            py::arg("data_spec"),
            py::arg("data_message_handler")
        )
        .def("create_rpc_client",
            [](py::object self,
                const std::string& canonicalName,
                SilKit::Services::Rpc::RpcSpec dataSpec,
                py::function pyHandler)
            {
                auto* participant = self.cast<SilKit::IParticipant*>();
                SilKitPy::CallbackRegistration registration(self, std::move(pyHandler));
                SilKit::Services::Rpc::IRpcClient* client;
                {
                    auto handler = registration.callback();
                    py::gil_scoped_release release;
                    client = participant->CreateRpcClient(canonicalName, dataSpec, std::move(handler));
                }
                registration.commit(SilKitPy::CallbackKey(client, "result"));
                return SilKitPy::BorrowWithOwner(client, self);
            },
            py::arg("canonical_name"),
            py::arg("data_spec"),
            py::arg("handler")
        )
        .def("create_rpc_server",
            [](py::object self,
                const std::string& canonicalName,
                SilKit::Services::Rpc::RpcSpec dataSpec,
                py::function pyHandler)
            {
                auto* participant = self.cast<SilKit::IParticipant*>();
                SilKitPy::CallbackRegistration registration(self, std::move(pyHandler));
                SilKit::Services::Rpc::IRpcServer* server;
                {
                    auto handler = registration.callback();
                    py::gil_scoped_release release;
                    server = participant->CreateRpcServer(canonicalName, dataSpec, std::move(handler));
                }
                registration.commit(SilKitPy::CallbackKey(server, "call"));
                return SilKitPy::BorrowWithOwner(server, self);
            },
            py::arg("canonical_name"),
            py::arg("data_spec"),
            py::arg("handler")
            )
        .def("get_logger", 
            [](py::object self) {
                return SilKitPy::BorrowWithOwner(self.cast<SilKit::IParticipant*>()->GetLogger(), self);
            });

    py::enum_<SilKit::Services::Logging::Level>(m, "LogLevel")
        .value("Trace", SilKit::Services::Logging::Level::Trace) 
        .value("Debug", SilKit::Services::Logging::Level::Debug) 
        .value("Info", SilKit::Services::Logging::Level::Info) 
        .value("Warn", SilKit::Services::Logging::Level::Warn) 
        .value("Error", SilKit::Services::Logging::Level::Error) 
        .value("Critical", SilKit::Services::Logging::Level::Critical) 
        .value("Off", SilKit::Services::Logging::Level::Off);

    py::class_<SilKit::Services::Logging::ILogger, py::smart_holder>(m, "ILogger", py::dynamic_attr())
        .def("log", 
            &SilKit::Services::Logging::ILogger::Log,
            py::arg("level"),
            py::arg("msg"),
            py::call_guard<py::gil_scoped_release>());

    bind_ICanController( m );

    bind_ILifecycleService( m );

    bind_PubSub( m );

    bind_Rpc( m );

    bind_MatchingLabel( m );

}
