#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include "PythonCallback.hpp"
#include "silkit/SilKit.hpp"

namespace py = pybind11;

struct LifecycleCompletion
{
    std::future<SilKit::Services::Orchestration::ParticipantState> future;
};

void bind_ILifecycleService(py::module_ &m)
    {
    py::class_<LifecycleCompletion>(m, "_LifecycleCompletion", py::dynamic_attr())
        .def("ready", [](LifecycleCompletion& completion) {
            // get() invalidates the future; wait_for on it would be undefined.
            if (!completion.future.valid())
                return true;
            return completion.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        })
        .def("result", [](LifecycleCompletion& completion) {
            if (!completion.future.valid())
                throw std::runtime_error("Lifecycle result was already retrieved");
            if (completion.future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
                throw std::runtime_error("Lifecycle has not completed");
            return completion.future.get();
        });
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
        py::smart_holder>(m, "ILifecycleService", py::dynamic_attr())
        .def("set_communication_ready_handler",  
            [](py::object self, py::function py_handler){
                auto* service = self.cast<SilKit::Services::Orchestration::ILifecycleService*>();
                SilKitPy::CallbackRegistration registration(self, std::move(py_handler));
                {
                    auto handler = registration.callback();
                    py::gil_scoped_release release;
                    service->SetCommunicationReadyHandler(std::move(handler));
                }
                registration.commit(SilKitPy::CallbackKey(service, "ready"));
            }
        )
        .def("set_communication_ready_handler_async",  
            [](py::object self, py::function py_handler){
                auto* service = self.cast<SilKit::Services::Orchestration::ILifecycleService*>();
                SilKitPy::CallbackRegistration registration(self, std::move(py_handler));
                {
                    auto handler = registration.callback();
                    py::gil_scoped_release release;
                    service->SetCommunicationReadyHandlerAsync(std::move(handler));
                }
                registration.commit(SilKitPy::CallbackKey(service, "ready"));
            }
        )
        .def("complete_communication_ready_handler_async",
            &SilKit::Services::Orchestration::ILifecycleService::CompleteCommunicationReadyHandlerAsync,
            py::call_guard<py::gil_scoped_release>())
        .def(
            "start_lifecycle",
            [](py::object self) {
                auto* service = self.cast<SilKit::Services::Orchestration::ILifecycleService*>();
                auto fut = [service] {
                    py::gil_scoped_release release;
                    return service->StartLifecycle();
                }();
                auto completion = py::cast(LifecycleCompletion{std::move(fut)});
                completion.attr("_silkit_owner") = self;
                return py::module_::import("SilKit_py._lifecycle").attr("wait_for_completion")(completion);
            },
            "Start lifecycle and return an awaitable. Cancelling it cancels the wait, not the simulation."
        )
        .def("stop", &SilKit::Services::Orchestration::ILifecycleService::Stop,
            py::call_guard<py::gil_scoped_release>())
        .def("state", &SilKit::Services::Orchestration::ILifecycleService::State,
            py::call_guard<py::gil_scoped_release>())
        .def("create_time_sync_service",
            [](py::object self) {
                auto* lifecycle = self.cast<SilKit::Services::Orchestration::ILifecycleService*>();
                SilKit::Services::Orchestration::ITimeSyncService* service;
                { py::gil_scoped_release release; service = lifecycle->CreateTimeSyncService(); }
                return SilKitPy::BorrowWithOwner(service, self);
            });


    py::class_<SilKit::Services::Orchestration::ITimeSyncService, py::smart_holder>(
        m, "ITimeSyncService", py::dynamic_attr()
    )
        .def(
            "set_simulation_step_handler",
            [](py::object self,
            py::function py_handler,
            py::object py_initial_step_size)
            {
                auto step_size_int = py_initial_step_size.cast<int64_t>();
                std::chrono::nanoseconds initial_step_size(step_size_int);

                auto* service = self.cast<SilKit::Services::Orchestration::ITimeSyncService*>();
                SilKitPy::CallbackRegistration registration(self, std::move(py_handler));
                {
                    auto handler = registration.callback();
                    py::gil_scoped_release release;
                    service->SetSimulationStepHandler(
                        [handler](std::chrono::nanoseconds now, std::chrono::nanoseconds duration) {
                            handler(now.count(), duration.count());
                        }, initial_step_size);
                }
                registration.commit(SilKitPy::CallbackKey(service, "step"));
            },
            py::arg("handler"),
            py::arg("initial_step_size")
        );

    };
