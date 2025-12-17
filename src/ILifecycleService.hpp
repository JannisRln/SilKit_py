#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include "silkit/SilKit.hpp"

namespace py = pybind11;

void bind_ILifecycleService(py::module_ &m)
    {
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
        .def("set_communication_ready_handler",  
            [](SilKit::Services::Orchestration::ILifecycleService& self, py::function py_handler){
                
                py::gil_scoped_acquire gil;
                py::function handler_copy = py_handler;

                self.SetCommunicationReadyHandler(
                    [handler_copy]( ){

                        py::gil_scoped_acquire gil;

                        try {
                            handler_copy();
                        }
                        catch (const py::error_already_set& e) {
                            PyErr_WriteUnraisable(e.value().ptr());
                        }
                    }
                );
            }
        )
        .def("set_communication_ready_handler_async",  
            [](SilKit::Services::Orchestration::ILifecycleService& self, py::function py_handler){
                
                py::gil_scoped_acquire gil;
                py::function handler_copy = py_handler;

                self.SetCommunicationReadyHandlerAsync(
                    [handler_copy]( ){

                        py::gil_scoped_acquire gil;

                        try {
                            handler_copy();
                        }
                        catch (const py::error_already_set& e) {
                            PyErr_WriteUnraisable(e.value().ptr());
                        }
                    }
                );
            }
        )
        .def("complete_communication_ready_handler_async",
            &SilKit::Services::Orchestration::ILifecycleService::CompleteCommunicationReadyHandlerAsync)
        .def(
            "start_lifecycle",
            [](SilKit::Services::Orchestration::ILifecycleService& self) {

                auto fut = self.StartLifecycle();

                auto wait_fn = [fut = std::move(fut)]() mutable {
                    py::gil_scoped_release release;
                    return fut.get();
                };

                py::object asyncio = py::module_::import("asyncio");
                py::object to_thread = asyncio.attr("to_thread");

                return to_thread(py::cpp_function(std::move(wait_fn)));
            },
            "Start lifecycle asynchronously. Returns an awaitable."
        )
        .def("stop", &SilKit::Services::Orchestration::ILifecycleService::Stop)
        .def("state", &SilKit::Services::Orchestration::ILifecycleService::State )
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

                py::function handler_copy = py_handler;

                self.SetSimulationStepHandler(
                    [handler_copy](std::chrono::nanoseconds now,
                                   std::chrono::nanoseconds duration)
                    {
                        py::gil_scoped_acquire gil;

                        try {
                            handler_copy(
                                now.count(),
                                duration.count()
                            );
                        }
                        catch (const py::error_already_set& e) {
                            PyErr_WriteUnraisable(e.value().ptr());
                        }
                    },
                    initial_step_size
                );
            },
            py::arg("handler"),
            py::arg("initial_step_size")
        );

    };