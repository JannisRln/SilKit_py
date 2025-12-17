#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include "silkit/SilKit.hpp"

namespace py = pybind11;

void bind_PubSub(py::module_ &m)
    {
    py::class_<SilKit::Services::PubSub::IDataPublisher, py::smart_holder>(
        m, "IDataPublisher")
    .def(
        "publish",
        [](SilKit::Services::PubSub::IDataPublisher* self, py::bytes py_data)
        {

            py::buffer_info info(py::buffer(py_data).request());

            if (info.itemsize != 1)
            {
                throw std::runtime_error("IDataPublisher.publish expects a bytes-like object");
            }

            const uint8_t* ptr = static_cast<const uint8_t*>(info.ptr);
            std::size_t size = static_cast<std::size_t>(info.size);

            SilKit::Util::Span<const uint8_t> span(ptr, size);

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
                return py::bytes(reinterpret_cast<const char*>(e.data.data()), e.data.size());
            },
            "Payload as bytes"
        );
    }