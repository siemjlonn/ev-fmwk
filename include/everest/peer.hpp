// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_PEER_HPP
#define EVEREST_PEER_HPP

#include <functional>
#include <future>
#include <list>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "types.hpp"
#include "utils/message_worker.hpp"
#include "utils/mqtt_io_base.hpp"
#include <everest/utils/helpers.hpp>

namespace everest {

class Peer {
public:
    Peer(const std::string& peer_id, mqtt::IOBase& mqtt_io);

    void publish_variable(const std::string& implementation_id, const std::string& variable_name, Value value);

    Value call_command(const std::string& other_peer_id, const std::string& implementation_id,
                       const std::string& command_name, const Arguments& arguments);

    UnsubscriptionCallback subscribe_variable(const std::string& other_peer_id, const std::string& implementation_id,
                                              const std::string& variable_name, const SubscriptionHandler& handler);

    void implement_command(const std::string& implementation_id, const std::string& command_name,
                           const CommandHandler& handler);

    UnsubscriptionCallback mqtt_subscribe(const std::string& topic, const MQTTSubscriptionHandler& handler);
    void mqtt_publish(const std::string& topic, const std::string& data);

    const std::string peer_id;

    using CallIDType = std::mt19937::result_type;

    class ExecutedCalls {
    public:
        using ExecutedCallMap = std::unordered_map<CallIDType, std::promise<Value>>;
        ExecutedCallMap::iterator get(std::mt19937& rng);
        void release(const ExecutedCallMap::iterator& it);
        bool set_result(CallIDType id, Value result);

    private:
        ExecutedCallMap map{};
        std::mutex mutex{};
    };

private:
    mqtt::IOBase& mqtt_io;

    std::mt19937 rng;

    utils::RegisteredHandlers<Value> json_handlers{};
    utils::RegisteredHandlers<std::string> string_handlers{};
    ExecutedCalls calls{};

    void handle_subscription(nlohmann::json message, const utils::TopicInfo& topic_info);
    void handle_command(nlohmann::json message, const utils::TopicInfo& topic_info);
    void handle_result(nlohmann::json message);
    void handle_external_mqtt(const std::string& topic, std::string message);
};

} // namespace everest

#endif // EVEREST_REQUEST_BROKER_HPP
