#include <pybind11/pybind11.h>
#include <pybind11/functional.h>

#include "silkit/SilKit.hpp"
#include <iostream>

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
                // Convert Python function → C++ handler
                SilKit::Services::PubSub::DataMessageHandler handler =
                    [pyHandler](SilKit::Services::PubSub::IDataSubscriber* sub,
                                const SilKit::Services::PubSub::DataMessageEvent& evt)
                    {
                        py::gil_scoped_acquire gil;

                        pyHandler(sub, evt);
                    };

                // Produce subscriber
                auto* subscriber =
                    self->CreateDataSubscriber(canonicalName, dataSpec, std::move(handler));

                return subscriber;
            },
            py::arg("canonical_name"),
            py::arg("data_spec"),
            py::arg("data_message_handler"),
            py::return_value_policy::reference,
            py::keep_alive<0, 3>()  // subscriber keeps Python callback alive
        )
        .def("get_logger", 
             &SilKit::IParticipant::GetLogger,
             py::return_value_policy::reference);

    py::enum_<SilKit::Services::Orchestration::OperationMode>(m, "OperationMode")
        .value("Invalid",      SilKit::Services::Orchestration::OperationMode::Invalid)
        .value("Coordinated",  SilKit::Services::Orchestration::OperationMode::Coordinated)
        .value("Autonomous",   SilKit::Services::Orchestration::OperationMode::Autonomous);

    py::class_<SilKit::Services::Orchestration::LifecycleConfiguration>(m, "LifecycleConfiguration")
        .def(py::init<>())
        .def(py::init<SilKit::Services::Orchestration::OperationMode>(),
            py::arg("operation_mode"))
        .def_readwrite("operation_mode",
            &SilKit::Services::Orchestration::LifecycleConfiguration::operationMode);

    py::class_<SilKit::Services::Orchestration::ILifecycleService,
        py::smart_holder>(m, "ILifecycleService")
        .def(
            "start_lifecycle",
            [](SilKit::Services::Orchestration::ILifecycleService& self) {
                // Capture the C++ StartLifecycle future
                auto fut = self.StartLifecycle();

                // Function that waits for the future WITHOUT holding the GIL
                auto wait_fn = [fut = std::move(fut)]() mutable {
                    // Release GIL for the duration of the blocking wait
                    py::gil_scoped_release release;
                    return fut.get();
                };

                // Wrap into asyncio.to_thread for Python-side awaiting
                py::object asyncio = py::module_::import("asyncio");
                py::object to_thread = asyncio.attr("to_thread");

                return to_thread(py::cpp_function(std::move(wait_fn)));
            },
            "Start lifecycle asynchronously. Returns an awaitable."
        )
        .def("stop", &SilKit::Services::Orchestration::ILifecycleService::Stop)
        .def("create_time_sync_service",
            &SilKit::Services::Orchestration::ILifecycleService::CreateTimeSyncService,
            py::return_value_policy::reference);


    py::class_<SilKit::Services::Orchestration::ITimeSyncService, py::smart_holder>(
        m, "ITimeSyncService"
    )
        .def(
            "set_simulation_step_handler",
            [](SilKit::Services::Orchestration::ITimeSyncService& self,
            py::function py_handler,
            py::object py_initial_step_size)
            {
                auto step_size_int = py_initial_step_size.cast<int64_t>();
                std::chrono::nanoseconds initial_step_size(step_size_int);
                // Keep the Python callback alive
                py::function handler_copy = py_handler;

                // Wrap Python function in a C++ lambda
                self.SetSimulationStepHandler(
                    [handler_copy](std::chrono::nanoseconds now,
                                   std::chrono::nanoseconds duration)
                    {
                        // Acquire GIL for calling Python
                        py::gil_scoped_acquire gil;

                        try {
                            handler_copy(
                                now.count(),       // pass integer nanoseconds
                                duration.count()
                            );
                        }
                        catch (const py::error_already_set& e) {
                            PyErr_WriteUnraisable(e.value().ptr()); // avoid crashing the C++ thread
                        }
                    },
                    initial_step_size
                );
            },
            py::arg("handler"),
            py::arg("initial_step_size")
        );

    py::class_<SilKit::Services::PubSub::IDataPublisher, py::smart_holder>(
        m, "IDataPublisher")
    .def(
        "publish",
        [](SilKit::Services::PubSub::IDataPublisher* self, py::bytes py_data)
        {
            // Extract buffer from Python bytes object
            py::buffer_info info(py::buffer(py_data).request());

            // Safety: must be uint8_t
            if (info.itemsize != 1)
            {
                throw std::runtime_error("IDataPublisher.publish expects a bytes-like object");
            }

            const uint8_t* ptr = static_cast<const uint8_t*>(info.ptr);
            std::size_t size = static_cast<std::size_t>(info.size);

            // Construct Span<const uint8_t>
            SilKit::Util::Span<const uint8_t> span(ptr, size);

            // Call underlying C++ Publish
            self->Publish(span);
        },
        py::arg("data"),
        "Publish raw bytes to the data publisher"
    );

    py::class_<SilKit::Services::PubSub::IDataSubscriber, py::smart_holder>(
        m, "IDataSubscriber");

    py::class_<SilKit::Services::PubSub::PubSubSpec>(m, "PubSubSpec")
        .def(py::init<>())
        .def(py::init<std::string, std::string>(),
             py::arg("topic"), py::arg("mediaType"))

        .def("add_label",
             (void (SilKit::Services::PubSub::PubSubSpec::*)(const SilKit::Services::MatchingLabel&))
                 &SilKit::Services::PubSub::PubSubSpec::AddLabel,
             py::arg("label"))

        .def("add_label",
             (void (SilKit::Services::PubSub::PubSubSpec::*)(const std::string&, const std::string&,
                                   SilKit::Services::MatchingLabel::Kind))
                 &SilKit::Services::PubSub::PubSubSpec::AddLabel,
             py::arg("key"), py::arg("value"), py::arg("kind"))

        .def_property_readonly("topic",
             &SilKit::Services::PubSub::PubSubSpec::Topic)

        .def_property_readonly("media_type",
             &SilKit::Services::PubSub::PubSubSpec::MediaType)

        .def_property_readonly("labels",
             &SilKit::Services::PubSub::PubSubSpec::Labels);

    py::class_<SilKit::Services::PubSub::DataMessageEvent>(m, "DataMessageEvent")
        .def_property_readonly(
            "timestamp",
            [](const SilKit::Services::PubSub::DataMessageEvent& e) {
                return static_cast<int64_t>(e.timestamp.count());
            },
            "Timestamp in nanoseconds"
        )
        .def_property_readonly(
            "data",
            [](const SilKit::Services::PubSub::DataMessageEvent& e) {
                // Convert Util::Span<const uint8_t> → Python bytes
                return py::bytes(reinterpret_cast<const char*>(e.data.data()), e.data.size());
            },
            "Payload as bytes"
        );

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
}
