#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include "silkit/SilKit.hpp"

namespace py = pybind11;

void bind_ICanController(py::module_ &m)
    {
    // -------------------- CAN enums / masks --------------------
    py::enum_<SilKit::Services::TransmitDirection>(m, "TransmitDirection", py::arithmetic())
        .value("Undefined", SilKit::Services::TransmitDirection::Undefined)
        .value("RX",        SilKit::Services::TransmitDirection::RX)
        .value("TX",        SilKit::Services::TransmitDirection::TX)
        .value("TXRX",      SilKit::Services::TransmitDirection::TXRX);

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

    py::enum_<SilKit::Services::Can::CanFrameFlag>(m, "CanFrameFlag", py::arithmetic())
        .value("Ide", SilKit::Services::Can::CanFrameFlag::Ide)
        .value("Rtr", SilKit::Services::Can::CanFrameFlag::Rtr)
        .value("Fdf", SilKit::Services::Can::CanFrameFlag::Fdf)
        .value("Brs", SilKit::Services::Can::CanFrameFlag::Brs)
        .value("Esi", SilKit::Services::Can::CanFrameFlag::Esi)
        .value("Xlf", SilKit::Services::Can::CanFrameFlag::Xlf)
        .value("Sec", SilKit::Services::Can::CanFrameFlag::Sec);

    py::enum_<SilKit::Services::Can::CanTransmitStatus>(m, "CanTransmitStatus", py::arithmetic())
        .value("Transmitted",       SilKit::Services::Can::CanTransmitStatus::Transmitted)
        .value("Canceled",          SilKit::Services::Can::CanTransmitStatus::Canceled)
        .value("TransmitQueueFull", SilKit::Services::Can::CanTransmitStatus::TransmitQueueFull);

    m.attr("CanTransmitStatus_DefaultMask") = py::int_(SilKit_CanTransmitStatus_DefaultMask);

    // -------------------- CAN event structs --------------------

    py::class_<SilKit::Services::Can::CanFrame>(m, "CanFrame")
    .def_readonly("can_id", &SilKit::Services::Can::CanFrame::canId)
    .def_property_readonly("data",
        [](const SilKit::Services::Can::CanFrame& f) {
            return py::bytes(
                reinterpret_cast<const char*>(f.dataField.data()),
                f.dataField.size());
        });

    py::class_<SilKit::Services::Can::CanFrameEvent>(m, "CanFrameEvent")
        .def_readonly("timestamp", &SilKit::Services::Can::CanFrameEvent::timestamp)
        .def_readonly("frame", &SilKit::Services::Can::CanFrameEvent::frame)
        .def_readonly("direction", &SilKit::Services::Can::CanFrameEvent::direction)
        .def_readonly("user_context", &SilKit::Services::Can::CanFrameEvent::userContext);

    py::class_<SilKit::Services::Can::ICanController, py::smart_holder>(m, "ICanController")
        .def("set_baud_rate",
            &SilKit::Services::Can::ICanController::SetBaudRate,
            py::arg("rate"), py::arg("fd_rate"), py::arg("xl_rate"))
        .def("reset", &SilKit::Services::Can::ICanController::Reset)
        .def("start", &SilKit::Services::Can::ICanController::Start)
        .def("stop", &SilKit::Services::Can::ICanController::Stop)
        .def("sleep", &SilKit::Services::Can::ICanController::Sleep)
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
                std::memcpy(data.data(), info.ptr, data.size());

                SilKit::Services::Can::CanFrame frame{};
                frame.canId = can_id;
                frame.flags = static_cast<SilKit::Services::Can::CanFrameFlagMask>(flags);
                frame.sdt = sdt;
                frame.vcid = vcid;
                frame.af = af;
                frame.dataField = SilKit::Util::Span<const uint8_t>(data.data(), data.size());

                if (py_dlc.is_none())
                    frame.dlc = static_cast<uint16_t>(data.size());
                else
                    frame.dlc = py_dlc.cast<uint16_t>();

                self.SendFrame(frame, reinterpret_cast<void*>(user_context));
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
            [](SilKit::Services::Can::ICanController& self,
            py::function py_handler,
            SilKit::Services::DirectionMask direction_mask)
            {
                py::function handler_copy = py_handler;
  
                auto id = self.AddFrameHandler( [handler_copy](SilKit::Services::Can::ICanController* ctrl,
                                const SilKit::Services::Can::CanFrameEvent& evt)
                    {
                        py::gil_scoped_acquire gil;
                        try { 
                            handler_copy(ctrl, evt);
                        }
                        catch (const py::error_already_set& e) 
                        { 
                            PyErr_WriteUnraisable(e.value().ptr()); 
                        }
                    },direction_mask);

                return py::int_(static_cast<std::uint64_t>(id));
            },
            py::arg("handler"),
            py::arg("direction_mask") =
                static_cast<SilKit::Services::DirectionMask>(SilKit::Services::TransmitDirection::RX));
    }

