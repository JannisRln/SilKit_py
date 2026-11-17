#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/native_enum.h>
#include <cstring>
#include <vector>
#include "CanDlc.hpp"
#include "PythonCallback.hpp"
#include "silkit/SilKit.hpp"

namespace py = pybind11;

void bind_ICanController(py::module_ &m)
    {
    // -------------------- CAN enums / masks --------------------
    py::native_enum<SilKit::Services::TransmitDirection>(m, "TransmitDirection", "enum.IntFlag")
        .value("Undefined", SilKit::Services::TransmitDirection::Undefined)
        .value("RX",        SilKit::Services::TransmitDirection::RX)
        .value("TX",        SilKit::Services::TransmitDirection::TX)
        .value("TXRX",      SilKit::Services::TransmitDirection::TXRX).finalize();

    py::enum_<SilKit::Services::Can::CanControllerState>(m, "CanControllerState")
        .value("Uninit",  SilKit::Services::Can::CanControllerState::Uninit)
        .value("Stopped", SilKit::Services::Can::CanControllerState::Stopped)
        .value("Started", SilKit::Services::Can::CanControllerState::Started)
        .value("Sleep",   SilKit::Services::Can::CanControllerState::Sleep);

    py::enum_<SilKit::Services::Can::CanErrorState>(m, "CanErrorState")
        .value("NotAvailable", SilKit::Services::Can::CanErrorState::NotAvailable)
        .value("ErrorActive",  SilKit::Services::Can::CanErrorState::ErrorActive)
        .value("ErrorPassive", SilKit::Services::Can::CanErrorState::ErrorPassive)
        .value("BusOff",       SilKit::Services::Can::CanErrorState::BusOff);

    py::native_enum<SilKit::Services::Can::CanFrameFlag>(m, "CanFrameFlag", "enum.IntFlag")
        .value("Ide", SilKit::Services::Can::CanFrameFlag::Ide)
        .value("Rtr", SilKit::Services::Can::CanFrameFlag::Rtr)
        .value("Fdf", SilKit::Services::Can::CanFrameFlag::Fdf)
        .value("Brs", SilKit::Services::Can::CanFrameFlag::Brs)
        .value("Esi", SilKit::Services::Can::CanFrameFlag::Esi)
        .value("Xlf", SilKit::Services::Can::CanFrameFlag::Xlf)
        .value("Sec", SilKit::Services::Can::CanFrameFlag::Sec).finalize();

    py::native_enum<SilKit::Services::Can::CanTransmitStatus>(m, "CanTransmitStatus", "enum.IntFlag")
        .value("Transmitted",       SilKit::Services::Can::CanTransmitStatus::Transmitted)
        .value("Canceled",          SilKit::Services::Can::CanTransmitStatus::Canceled)
        .value("TransmitQueueFull", SilKit::Services::Can::CanTransmitStatus::TransmitQueueFull).finalize();

    m.attr("CanTransmitStatus_DefaultMask") = py::int_(SilKit_CanTransmitStatus_DefaultMask);

    // -------------------- CAN event structs --------------------

    py::class_<SilKit::Services::Can::CanFrame>(m, "CanFrame")
    .def_readonly("can_id", &SilKit::Services::Can::CanFrame::canId)
    .def_readonly("flags", &SilKit::Services::Can::CanFrame::flags)
    .def_readonly("dlc", &SilKit::Services::Can::CanFrame::dlc)
    .def_readonly("sdt", &SilKit::Services::Can::CanFrame::sdt)
    .def_readonly("vcid", &SilKit::Services::Can::CanFrame::vcid)
    .def_readonly("af", &SilKit::Services::Can::CanFrame::af)
    .def_property_readonly("data",
        [](const SilKit::Services::Can::CanFrame& f) {
            return py::bytes(
                reinterpret_cast<const char*>(f.dataField.data()),
                f.dataField.size());
        });

    py::class_<SilKit::Services::Can::CanFrameEvent,
        std::shared_ptr<SilKit::Services::Can::CanFrameEvent>>(m, "CanFrameEvent")
        .def_property_readonly("timestamp", [](const SilKit::Services::Can::CanFrameEvent& event) {
            return event.timestamp.count();
        }, "Timestamp in nanoseconds")
        .def_readonly("frame", &SilKit::Services::Can::CanFrameEvent::frame)
        .def_readonly("direction", &SilKit::Services::Can::CanFrameEvent::direction)
        .def_property_readonly("user_context", [](const SilKit::Services::Can::CanFrameEvent& event) {
            return reinterpret_cast<uintptr_t>(event.userContext);
        });

    py::class_<SilKit::Services::Can::ICanController, py::smart_holder>(m, "ICanController", py::dynamic_attr())
        .def("set_baud_rate",
            &SilKit::Services::Can::ICanController::SetBaudRate,
            py::arg("rate"), py::arg("fd_rate"), py::arg("xl_rate"),
            py::call_guard<py::gil_scoped_release>())
        .def("reset", &SilKit::Services::Can::ICanController::Reset,
            py::call_guard<py::gil_scoped_release>())
        .def("start", &SilKit::Services::Can::ICanController::Start,
            py::call_guard<py::gil_scoped_release>())
        .def("stop", &SilKit::Services::Can::ICanController::Stop,
            py::call_guard<py::gil_scoped_release>())
        .def("sleep", &SilKit::Services::Can::ICanController::Sleep,
            py::call_guard<py::gil_scoped_release>())
        .def("send_frame",
            [](SilKit::Services::Can::ICanController& self,
            uint32_t can_id,
            py::bytes py_data,
            uint32_t flags,
            py::object py_dlc,
            uint8_t sdt,
            uint8_t vcid,
            uint32_t af,
            uintptr_t user_context)
            {

                py::buffer_info info(py::buffer(py_data).request());
                if (info.itemsize != 1) { 
                    throw std::runtime_error("send_frame expects bytes-like payload"); 
                }

                std::vector<uint8_t> data(static_cast<size_t>(info.size));
                if (!data.empty())
                    std::memcpy(data.data(), info.ptr, data.size());

                SilKit::Services::Can::CanFrame frame{};
                frame.canId = can_id;
                frame.flags = static_cast<SilKit::Services::Can::CanFrameFlagMask>(flags);
                frame.sdt = sdt;
                frame.vcid = vcid;
                frame.af = af;
                frame.dataField = SilKit::Util::Span<const uint8_t>(data.data(), data.size());

                if (py_dlc.is_none())
                    frame.dlc = SilKitPy::InferCanDlc(
                        data.size(),
                        (flags & static_cast<uint32_t>(SilKit::Services::Can::CanFrameFlag::Fdf)) != 0,
                        (flags & static_cast<uint32_t>(SilKit::Services::Can::CanFrameFlag::Xlf)) != 0);
                else
                    frame.dlc = py_dlc.cast<uint16_t>();

                {
                    py::gil_scoped_release release;
                    self.SendFrame(frame, reinterpret_cast<void*>(user_context));
                }
            },
            py::arg("can_id"),
            py::arg("data"),
            py::arg("flags") = 0u,
            py::arg("dlc") = py::none(),
            py::arg("sdt") = 0,
            py::arg("vcid") = 0,
            py::arg("af") = 0u,
            py::arg("user_context") = 0u)
        .def("add_frame_handler",
            [](py::object self,
            py::function py_handler,
            SilKit::Services::DirectionMask direction_mask)
            {
                auto* controller = self.cast<SilKit::Services::Can::ICanController*>();
                SilKitPy::CallbackRegistration registration(self, std::move(py_handler));
                SilKit::Util::HandlerId id;
                {
                    auto handler = registration.callback();
                    py::gil_scoped_release release;
                    id = controller->AddFrameHandler(std::move(handler), direction_mask);
                }
                registration.commit(SilKitPy::CallbackKey(controller, "frame", static_cast<uint64_t>(id)));
                return static_cast<uint64_t>(id);
            },
            py::arg("handler"),
            py::arg("direction_mask") =
                static_cast<SilKit::Services::DirectionMask>(SilKit::Services::TransmitDirection::RX))
        .def("remove_frame_handler", [](py::object self, uint64_t id) {
            auto* controller = self.cast<SilKit::Services::Can::ICanController*>();
            { py::gil_scoped_release release; controller->RemoveFrameHandler(static_cast<SilKit::Util::HandlerId>(id)); }
            auto callbacks = SilKitPy::CallbackRegistry(SilKitPy::ParticipantOwner(self));
            callbacks.attr("pop")(SilKitPy::CallbackKey(controller, "frame", id), py::none());
        }, py::arg("handler_id"));
    }

