// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_MODULE_PEER_HPP
#define EVEREST_MODULE_PEER_HPP

#include <functional>
#include <future>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "module.hpp"

namespace everest {

// forward declarations
namespace mqtt {
class IOBase;
}
class Peer;

class ModulePeer {
public:
    enum class State
    {
        Constructed,
        Booted,
        Initialized,
    };

    ModulePeer() = delete;
    explicit ModulePeer(const std::string& module_id);
    explicit ModulePeer(const Module& module);
    ModulePeer(const Module& module, std::unique_ptr<mqtt::IOBase> mqtt_io);
    ~ModulePeer();

    const Module& get_module() const;

    //
    // the following 4 function do not check any of it's argument and should only be used by trusted users, and hence
    // should be used with care - for now we'll leave them public, but they really should be private
    //
    Value call_command(const Fulfillment& fulfillment, const std::string& command_name, const Arguments& args) const;
    void publish_variable(const std::string& implementation_id, const std::string& variable_name,
                          const Value& value) const;

    void implement_command(const std::string& implementation_id, const std::string& command_name,
                           const CommandHandler& handler) const;

    UnsubscriptionCallback subscribe_variable(const Fulfillment& fulfillment, const std::string& variable_name,
                                              const SubscriptionHandler& handler) const;

    UnsubscriptionCallback mqtt_subscribe_variable(const std::string& topic, const MQTTSubscriptionHandler& handler);
    void mqtt_publish_variable(const std::string& topic, const std::string& data);

    const ModuleConfig& say_hello();
    void init_done();
    void bypass(const nlohmann::json& module_config);

    std::future<void> spawn_io_sync_thread();

private:
    Module module;
    ModuleConfig config;
    std::unique_ptr<mqtt::IOBase> mqtt_io{nullptr};
    std::unique_ptr<Peer> peer;

    bool bypassed{false};

    // FIXME (aw): this should be put into the state, if somehow possible
    bool sync_thread_running{false};
    std::thread sync_thread;

    State state{State::Constructed};

    friend class ModuleImplementationBuilder;
    friend class ModuleSubscriptionBuilder;
};
} // namespace everest

#endif // EVEREST_MODULE_PEER_HPP
