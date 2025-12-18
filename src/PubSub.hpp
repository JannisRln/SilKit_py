#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "silkit/SilKit.hpp"
#include "silkit/util/serdes/Serialization.hpp"

namespace py = pybind11;

void bind_PubSub(py::module_ &m)
    {

    m.def(
        "media_type_data",
        &SilKit::Util::SerDes::MediaTypeData,
        "The Data media / mime type the serializer / deserializer can be used for"
    );

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

    };

void bind_Rpc(py::module_ &m)
    {

    m.def(
        "media_type_rpc",
        &SilKit::Util::SerDes::MediaTypeRpc,
        "The RPC media / mime type the serializer / deserializer can be used for"
    );

    py::enum_<SilKit::Services::Rpc::RpcCallStatus>(m, "RpcCallStatus")
        .value("Success",             SilKit::Services::Rpc::RpcCallStatus::Success, "Call was successful.")
        .value("ServerNotReachable",  SilKit::Services::Rpc::RpcCallStatus::ServerNotReachable, "No server matching the RpcSpec was found.")
        .value("UndefinedError",      SilKit::Services::Rpc::RpcCallStatus::UndefinedError ,"An unidentified error occured.")
        .value("InternalServerError", SilKit::Services::Rpc::RpcCallStatus::InternalServerError, "The Call lead to an internal RpcServer error. This might happen if no CallHandler was specified for the RpcServer.")
        .value("Timeout",             SilKit::Services::Rpc::RpcCallStatus::Timeout, "The Call did run into a timeout and was canceled. This might happen if a corresponding server crashed, ran into an error or took too long to answer the call.");

    py::class_<SilKit::Services::Rpc::RpcSpec>(m, "RpcSpec")
        .def(py::init<>())
        .def(py::init<std::string, std::string>(),
             py::arg("function_name"), py::arg("mediaType"))

        .def("add_label",
             (void (SilKit::Services::Rpc::RpcSpec::*)(const SilKit::Services::MatchingLabel&))
                 &SilKit::Services::Rpc::RpcSpec::AddLabel,
             py::arg("label"))

        .def("add_label",
             (void (SilKit::Services::Rpc::RpcSpec::*)(const std::string&, const std::string&,
                                   SilKit::Services::MatchingLabel::Kind))
                 &SilKit::Services::Rpc::RpcSpec::AddLabel,
             py::arg("key"), py::arg("value"), py::arg("kind"))

        .def_property_readonly("function_name",
             &SilKit::Services::Rpc::RpcSpec::FunctionName)

        .def_property_readonly("media_type",
             &SilKit::Services::Rpc::RpcSpec::MediaType)

        .def_property_readonly("labels",
             &SilKit::Services::Rpc::RpcSpec::Labels);

    py::class_<SilKit::Services::Rpc::RpcCallResultEvent>(m, "RpcCallResultEvent")
        .def_property_readonly(
            "timestamp_ns",
            [](const SilKit::Services::Rpc::RpcCallResultEvent& e) {
                return e.timestamp.count();
            },
            "Send timestamp of the event in nanoseconds"
        )
        .def_property_readonly(
            "user_context",
            [](const SilKit::Services::Rpc::RpcCallResultEvent& e) {
                return reinterpret_cast<std::uintptr_t>(e.userContext);
            },
            "User context pointer as integer"
        )
        .def_readonly(
            "call_status",
            &SilKit::Services::Rpc::RpcCallResultEvent::callStatus,
            "Status of the RPC call"
        )
        .def_property_readonly(
            "result_data",
            [](const SilKit::Services::Rpc::RpcCallResultEvent& e) {
                return py::bytes(
                    reinterpret_cast<const char*>(e.resultData.data()),
                    e.resultData.size()
                );
            },
            "RPC call result data (valid only if call_status == Success)"
        );

    py::class_<SilKit::Services::Rpc::RpcCallEvent>(m, "RpcCallEvent")
        .def_property_readonly(
            "timestamp_ns",
            [](const SilKit::Services::Rpc::RpcCallEvent& e) {
                return e.timestamp.count();
            },
            "Send timestamp of the event in nanoseconds"
        )
        .def_property_readonly(
            "call_handle",
            [](const SilKit::Services::Rpc::RpcCallEvent& e) {
                return reinterpret_cast<std::uintptr_t>(e.callHandle);
            },
            "RPC call handle as opaque integer"
        )
        .def_property_readonly(
            "argument_data",
            [](const SilKit::Services::Rpc::RpcCallEvent& e) {
                return py::bytes(
                    reinterpret_cast<const char*>(e.argumentData.data()),
                    e.argumentData.size()
                );
            },
            "RPC call argument data"
        );

    py::class_<SilKit::Services::Rpc::IRpcClient>(m, "IRpcClient")
        .def(
            "call",
            [](SilKit::Services::Rpc::IRpcClient& self,
               py::bytes data,
               py::object user_context) {
                std::cout << "IRpcClient.call" << std::endl;
                std::string buffer = data;
                void* ctx = user_context.is_none()
                    ? nullptr
                    : reinterpret_cast<void*>(
                          user_context.cast<std::uintptr_t>());

                self.Call(
                    SilKit::Util::Span<const uint8_t>(
                        reinterpret_cast<const uint8_t*>(buffer.data()),
                        buffer.size()),
                    ctx);
            },
            py::arg("data"),
            py::arg("user_context") = py::none(),
            "Initiate a remote procedure call"
        )
        .def(
            "call_with_timeout",
            [](SilKit::Services::Rpc::IRpcClient& self,
               py::bytes data,
               std::uint64_t timeout_ns,
               py::object user_context) {
                std::cout << "IRpcClient.call_with_timeout" << std::endl;
                std::string buffer = data;
                void* ctx = user_context.is_none()
                    ? nullptr
                    : reinterpret_cast<void*>(
                          user_context.cast<std::uintptr_t>());

                self.CallWithTimeout(
                    SilKit::Util::Span<const uint8_t>(
                        reinterpret_cast<const uint8_t*>(buffer.data()),
                        buffer.size()),
                    std::chrono::nanoseconds(timeout_ns),
                    ctx);
            },
            py::arg("data"),
            py::arg("timeout_ns"),
            py::arg("user_context") = py::none(),
            "Initiate a remote procedure call with timeout (nanoseconds)"
        )
        .def(
            "set_call_result_handler",
            [](SilKit::Services::Rpc::IRpcClient& self, py::function py_handler) {
                self.SetCallResultHandler(
                    [py_handler](SilKit::Services::Rpc::IRpcClient* client,
                                 const SilKit::Services::Rpc::RpcCallResultEvent& event) {
                        py::gil_scoped_acquire gil;
                        py_handler(client, event);
                    });
            },
            py::arg("handler"),
            "Set the RPC call result handler"
        );
        
    py::class_<SilKit::Services::Rpc::IRpcServer>(m, "IRpcServer")
        .def(
            "submit_result",
            [](SilKit::Services::Rpc::IRpcServer& self,
               std::uintptr_t call_handle,
               py::bytes result_data) {
                std::string buffer = result_data;

                self.SubmitResult(
                    reinterpret_cast<SilKit::Services::Rpc::IRpcCallHandle*>(call_handle),
                    SilKit::Util::Span<const uint8_t>(
                        reinterpret_cast<const uint8_t*>(buffer.data()),
                        buffer.size()));
            },
            py::arg("call_handle"),
            py::arg("result_data"),
            "Submit the result of an RPC call"
        )
        .def(
            "set_call_handler",
            [](SilKit::Services::Rpc::IRpcServer& self, py::function py_handler) {
                self.SetCallHandler(
                    [py_handler](SilKit::Services::Rpc::IRpcServer* server,
                                 const SilKit::Services::Rpc::RpcCallEvent& event) {
                        py::gil_scoped_acquire gil;
                        py_handler(server, event);
                    });
            },
            py::arg("handler"),
            "Set the RPC call handler"
        );

    }


void bind_MatchingLabel(py::module_& m)
{
    py::enum_<SilKit::Services::MatchingLabel::Kind>(m, "MatchingLabelKind")
        .value("Optional",  SilKit::Services::MatchingLabel::Kind::Optional)
        .value("Mandatory", SilKit::Services::MatchingLabel::Kind::Mandatory);

    py::class_<SilKit::Services::MatchingLabel>(m, "MatchingLabel")
        .def(py::init<>())
        .def(
            py::init<std::string, std::string, SilKit::Services::MatchingLabel::Kind>(),
            py::arg("key"),
            py::arg("value"),
            py::arg("kind"),
            "Create a MatchingLabel with key, value, and matching kind"
        )
        .def_readwrite(
            "key",
            &SilKit::Services::MatchingLabel::key,
            "The label key"
        )
        .def_readwrite(
            "value",
            &SilKit::Services::MatchingLabel::value,
            "The label value"
        )
        .def_readwrite(
            "kind",
            &SilKit::Services::MatchingLabel::kind,
            "The matching kind for this label"
        );
}