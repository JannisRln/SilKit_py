#pragma once

#include <pybind11/pybind11.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <utility>
#include <thread>
#include <vector>
#include "OwnedEvents.hpp"

namespace SilKitPy {

namespace py = pybind11;

// Keep ownership visible to Python's cyclic garbage collector.
template <typename T>
py::object BorrowWithOwner(T* value, py::handle owner)
{
    auto result = py::cast(value, py::return_value_policy::reference);
    result.attr("_silkit_owner") = owner;
    return result;
}

inline py::object ParticipantOwner(py::object service)
{
    while (py::hasattr(service, "_silkit_owner"))
        service = service.attr("_silkit_owner");
    return service;
}

inline py::dict CallbackRegistry(py::handle participant)
{
    if (!py::hasattr(participant, "_silkit_callbacks"))
        participant.attr("_silkit_callbacks") = py::dict();
    return participant.attr("_silkit_callbacks").cast<py::dict>();
}

inline py::tuple CallbackKey(const void* service, const char* kind, uint64_t id = 0)
{
    return py::make_tuple(reinterpret_cast<uintptr_t>(service), kind, id);
}

struct CallbackState {};

// ---------------------------------------------------------------------------
// Interpreter shutdown
//
// Once Py_FinalizeEx has started, a foreign thread that tries to take the GIL
// never returns. The participant destructor joins exactly those threads, so a
// single callback during shutdown hangs the process. An atexit hook (it runs
// before finalization starts) turns all callbacks into no-ops, stops running
// simulations and waits for the callbacks already in flight.
// ---------------------------------------------------------------------------
struct ShutdownState
{
    std::atomic<bool> stopping{false};
    std::atomic<int> inFlight{0};
    std::thread::id exitThread;
    // Lifecycle services of live participants. Guarded by lifecycleMutex.
    std::mutex lifecycleMutex;
    std::vector<std::pair<SilKit::IParticipant*, SilKit::Services::Orchestration::ILifecycleService*>> lifecycles;
};

inline ShutdownState& Shutdown()
{
    // Leaked deliberately: native threads may still read it while this
    // module's static objects are being destroyed.
    static auto* state = new ShutdownState();
    return *state;
}

inline bool IsShuttingDown() { return Shutdown().stopping.load(); }

// Counts a native thread that may touch Python state, so shutdown can wait.
struct InFlightGuard
{
    InFlightGuard() { Shutdown().inFlight.fetch_add(1); }
    ~InFlightGuard() { Shutdown().inFlight.fetch_sub(1); }
    InFlightGuard(const InFlightGuard&) = delete;
    InFlightGuard& operator=(const InFlightGuard&) = delete;
};

inline void RegisterLifecycle(SilKit::IParticipant* participant,
                              SilKit::Services::Orchestration::ILifecycleService* lifecycle)
{
    auto& state = Shutdown();
    std::lock_guard<std::mutex> lock(state.lifecycleMutex);
    state.lifecycles.emplace_back(participant, lifecycle);
}

inline void UnregisterLifecycles(SilKit::IParticipant* participant)
{
    auto& state = Shutdown();
    std::lock_guard<std::mutex> lock(state.lifecycleMutex);
    auto& entries = state.lifecycles;
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                      [participant](const auto& entry) { return entry.first == participant; }),
        entries.end());
}

// Once callbacks are no-ops, a running time-synchronized simulation advances
// without pause, and SIL Kit's participant destructor does not return while
// it does. Stop running simulations before finalization destroys them.
inline void StopRunningLifecycles(std::chrono::steady_clock::time_point deadline)
{
    using SilKit::Services::Orchestration::ParticipantState;
    auto isActive = [](ParticipantState state) {
        return state == ParticipantState::Running || state == ParticipantState::Paused
            || state == ParticipantState::Stopping || state == ParticipantState::Stopped
            || state == ParticipantState::ShuttingDown;
    };

    auto& state = Shutdown();
    // Held throughout, so no participant can be deleted while it is stopped.
    std::lock_guard<std::mutex> lock(state.lifecycleMutex);
    for (auto& entry : state.lifecycles) {
        try {
            auto current = entry.second->State();
            if (current == ParticipantState::Running || current == ParticipantState::Paused)
                entry.second->Stop("Python interpreter is exiting");
        }
        catch (...) {
        }
    }
    for (auto& entry : state.lifecycles) {
        try {
            while (isActive(entry.second->State()) && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        catch (...) {
        }
    }
}

inline void OnPythonExit()
{
    auto& state = Shutdown();
    state.exitThread = std::this_thread::get_id();
    state.stopping.store(true);
    py::gil_scoped_release release;
    // Neither a stuck simulation nor a callback blocked in user code may keep
    // the interpreter alive forever.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    StopRunningLifecycles(deadline);
    while (state.inFlight.load() != 0 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

inline void RegisterShutdownHook()
{
    py::module_::import("atexit").attr("register")(py::cpp_function(&OnPythonExit));
}

// ---------------------------------------------------------------------------
// Participant destruction
// ---------------------------------------------------------------------------
inline unsigned& CallbackDepth()
{
    static thread_local unsigned depth = 0;
    return depth;
}

// Deletions that reached the main thread while it was itself inside a
// callback, i.e. still inside a native SIL Kit call stack.
inline std::vector<SilKit::IParticipant*>& DeferredDeletes()
{
    static thread_local std::vector<SilKit::IParticipant*> deletes;
    return deletes;
}

inline void ScheduleParticipantDelete(SilKit::IParticipant* value);

// Must be called without the GIL: the destructor joins SIL Kit's threads.
inline void DeleteParticipant(SilKit::IParticipant* value)
{
    UnregisterLifecycles(value);
    delete value;
}

struct CallbackScope
{
    CallbackScope() { ++CallbackDepth(); }
    ~CallbackScope()
    {
        if (--CallbackDepth() != 0 || DeferredDeletes().empty()) return;
        // The native caller is still on the stack here: hand the deletions
        // back to the next bytecode boundary instead of running them now.
        auto deletes = std::move(DeferredDeletes());
        DeferredDeletes().clear();
        for (auto* value : deletes)
            ScheduleParticipantDelete(value);
    }
    CallbackScope(const CallbackScope&) = delete;
    CallbackScope& operator=(const CallbackScope&) = delete;
};

// The last owner can disappear when a Python callback returns. SIL Kit must
// not join its own I/O thread from that thread's callback stack.
struct ParticipantDeleter
{
    static int DeleteOnMainThread(void* data)
    {
        auto* value = static_cast<SilKit::IParticipant*>(data);
        // Pending calls run at any bytecode boundary, including inside a
        // callback that SIL Kit invoked synchronously on the main thread.
        if (CallbackDepth() != 0) {
            DeferredDeletes().push_back(value);
            return 0;
        }
        py::gil_scoped_release release;
        DeleteParticipant(value);
        return 0;
    }

    void operator()(SilKit::IParticipant* value) const
    {
        if (CallbackDepth() != 0)
            ScheduleParticipantDelete(value);
        else
            DeleteParticipant(value);
    }
};

inline void ScheduleParticipantDelete(SilKit::IParticipant* value)
{
    if (Py_AddPendingCall(ParticipantDeleter::DeleteOnMainThread, value) != 0) {
        // A full pending-call queue must not cause a self-join either.
        std::thread([value] { DeleteParticipant(value); }).detach();
    }
}

template <typename T>
py::object CallbackArgument(T value, py::handle) { return py::cast(value); }
template <typename T>
py::object CallbackArgument(T* value, py::handle owner) { return BorrowWithOwner(value, owner); }
inline py::object CallbackArgument(const SilKit::Services::Can::CanFrameEvent& e, py::handle)
{ return py::cast(OwnEvent(e)); }
inline py::object CallbackArgument(const SilKit::Services::PubSub::DataMessageEvent& e, py::handle)
{ return py::cast(OwnEvent(e)); }
inline py::object CallbackArgument(const SilKit::Services::Rpc::RpcCallEvent& e, py::handle)
{ return py::cast(OwnEvent(e)); }
inline py::object CallbackArgument(const SilKit::Services::Rpc::RpcCallResultEvent& e, py::handle)
{ return py::cast(OwnEvent(e)); }

// Native handlers retain only a weak reference to GC-visible callback state.
class PythonCallback
{
public:
    explicit PythonCallback(py::handle state)
        : _state(new py::weakref(state), &ReleaseState)
    {
    }

    template <typename... Args>
    void operator()(Args&&... args) const noexcept
    {
        InFlightGuard inFlight;
        if (IsShuttingDown()) return;
        CallbackScope callbackScope;
        pybind11::gil_scoped_acquire acquire;
        // Shutdown may have begun while this thread waited for the GIL.
        if (IsShuttingDown()) return;
        try {
            auto state = (*_state)();
            if (state.is_none()) return;
            auto owner = state.attr("owner")();
            if (owner.is_none()) return;
            auto function = state.attr("function");
            try {
                function(CallbackArgument(std::forward<Args>(args), owner)...);
            }
            catch (pybind11::error_already_set& error) {
                error.discard_as_unraisable(function);
            }
        }
        // Nothing may escape into SIL Kit's threads; that would terminate.
        catch (pybind11::error_already_set& error) {
            error.discard_as_unraisable("SilKit_py callback");
        }
        catch (const std::exception& error) {
            ReportUnraisable(error.what());
        }
        catch (...) {
            ReportUnraisable("unknown C++ exception in SilKit_py callback");
        }
    }

private:
    static void ReportUnraisable(const char* message) noexcept
    {
        PyErr_SetString(PyExc_RuntimeError, message);
        PyErr_WriteUnraisable(nullptr);
    }

    static void ReleaseState(py::weakref* state) noexcept
    {
        auto& shutdown = Shutdown();
        if (shutdown.stopping.load() && std::this_thread::get_id() != shutdown.exitThread) {
            // A foreign thread can no longer take the GIL. Leak the reference.
            state->release();
            delete state;
            return;
        }
        pybind11::gil_scoped_acquire acquire;
        delete state;
    }

    std::shared_ptr<py::weakref> _state;
};

// Commit only after native registration succeeds; otherwise release locally.
class CallbackRegistration
{
public:
    CallbackRegistration(py::object service, py::function function)
        : _owner(ParticipantOwner(std::move(service))), _state(py::cast(CallbackState{}))
    {
        _state.attr("owner") = py::weakref(_owner);
        _state.attr("function") = std::move(function);
    }
    PythonCallback callback() const { return PythonCallback(_state); }
    void commit(py::handle key) { CallbackRegistry(_owner)[key] = _state; }

private:
    py::object _owner;
    py::object _state;
};

} // namespace SilKitPy
