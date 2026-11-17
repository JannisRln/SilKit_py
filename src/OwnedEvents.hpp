#pragma once

#include "silkit/SilKit.hpp"
#include <memory>
#include <vector>

namespace SilKitPy {

template <typename Event>
struct EventStorage
{
    Event event;
    std::vector<uint8_t> payload;

    EventStorage(const Event& source, SilKit::Util::Span<const uint8_t> data)
        : event(source)
    {
        if (!data.empty()) payload.assign(data.begin(), data.end());
    }
};

template <typename Event, typename SetSpan>
std::shared_ptr<Event> OwnEvent(const Event& event, SilKit::Util::Span<const uint8_t> data, SetSpan setSpan)
{
    auto storage = std::make_shared<EventStorage<Event>>(event, data);
    setSpan(storage->event, SilKit::Util::Span<const uint8_t>(storage->payload.data(), storage->payload.size()));
    return std::shared_ptr<Event>(storage, &storage->event);
}

inline auto OwnEvent(const SilKit::Services::Can::CanFrameEvent& event)
{
    return OwnEvent(event, event.frame.dataField, [](auto& e, auto span) { e.frame.dataField = span; });
}
inline auto OwnEvent(const SilKit::Services::PubSub::DataMessageEvent& event)
{
    return OwnEvent(event, event.data, [](auto& e, auto span) { e.data = span; });
}
inline auto OwnEvent(const SilKit::Services::Rpc::RpcCallEvent& event)
{
    return OwnEvent(event, event.argumentData, [](auto& e, auto span) { e.argumentData = span; });
}
inline auto OwnEvent(const SilKit::Services::Rpc::RpcCallResultEvent& event)
{
    return OwnEvent(event, event.resultData, [](auto& e, auto span) { e.resultData = span; });
}

} // namespace SilKitPy
