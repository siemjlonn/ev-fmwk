// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest

#include <everest/peer.hpp>

#include <functional>

#include <fmt/core.h>

#include <everest/constants.hpp>
#include <everest/logging/logging.hpp>
#include <everest/utils/mqtt_io_mqttc.hpp>

namespace everest {

inline std::mt19937::result_type string_to_seed(const std::string& value) {
    // FIXME (aw): we could also add the time here
    return std::hash<std::string>()(value) % std::numeric_limits<std::mt19937::result_type>::max();
}

inline std::string get_subscription_key(const std::string& peer_id, const std::string& impl_id) {
    if (impl_id.empty()) {
        return fmt::format("var/{}", peer_id);
    } else {
        return fmt::format("var/{}/{}", peer_id, impl_id);
    }
}

inline std::string get_command_key(const std::string& impl_id) {
    if (impl_id.empty()) {
        return fmt::format("cmd/");
    } else {
        return fmt::format("cmd/{}", impl_id);
    }
}

Peer::Peer(const std::string& peer_id, mqtt::IOBase& mqtt_io) :
    peer_id(peer_id), mqtt_io(mqtt_io), rng(string_to_seed(peer_id)) {

    this->mqtt_io.set_handler([this](const mqtt::RawMessage& raw_msg) {
        // first check topic
        const std::string topic(raw_msg.topic, raw_msg.topic_len);
        const auto topic_info = utils::parse_topic(topic);

        if (topic_info.type == utils::TopicInfo::INVALID) {
            // this should not be possible
            // FIXME (aw): exception handling
            throw std::runtime_error("Received data on invalid topic");
        } else if (topic_info.type == utils::TopicInfo::OTHER) {
            // this should be something else (external mqtt)
            this->handle_external_mqtt(topic, std::string(raw_msg.payload, raw_msg.payload_len));
            return;
        }

        auto message = nlohmann::json::parse(raw_msg.payload, raw_msg.payload + raw_msg.payload_len, nullptr, false);

        // EVLOG(error) << "Topic: " << topic << "\nMsg: " << message.dump(2);

        if (message.is_discarded()) {
            EVLOG(warning) << "Received unparseable message";
            return;
        }

        if (topic_info.type == utils::TopicInfo::RESULT) {
            this->handle_result(std::move(message));
        } else if (topic_info.type == utils::TopicInfo::VAR) {
            this->handle_subscription(std::move(message), topic_info);
        } else if (topic_info.type == utils::TopicInfo::CMD) {
            this->handle_command(std::move(message), topic_info);
        }
    });

    // subscribe to our result topic, we could this do lazily but with more complexity
    this->mqtt_io.subscribe(utils::get_result_topic(this->peer_id), mqtt::QOS::QOS1);
}

void Peer::publish_variable(const std::string& implementation_id, const std::string& variable_name, Value value) {
    const auto topic = utils::get_variable_topic(this->peer_id, implementation_id, variable_name);

    // FIXME (aw): it is not clear yet, if we need QOS2
    this->mqtt_io.publish(topic, value.dump(), mqtt::QOS::QOS2);
}

Value Peer::call_command(const std::string& other_peer_id, const std::string& implementation_id,
                         const std::string& command_name, const Arguments& arguments) {

    // allocate new call
    auto call = this->calls.get(this->rng);
    const auto& call_id = call->first;

    // get the future
    auto call_result_future = std::move(call->second.get_future());

    // setup the call packet
    const auto call_json = nlohmann::json({
        {"params", arguments},
        {"peer", this->peer_id},
        {"id", call_id},
    });

    const auto topic = utils::get_command_topic(other_peer_id, implementation_id, command_name);

    // send the call
    this->mqtt_io.publish(topic, call_json.dump(), mqtt::QOS::QOS2);

    auto status = call_result_future.wait_for(std::chrono::milliseconds(defaults::CALL_TIMEOUT_MS));

    calls.release(call);

    if (status == std::future_status::timeout) {
        // FIXME (aw): exception handling
        throw std::runtime_error(fmt::format("Command on path '{}' timeout out", topic));
    }

    return call_result_future.get();
}

UnsubscriptionCallback Peer::subscribe_variable(const std::string& other_peer_id, const std::string& implementation_id,
                                                const std::string& variable_name, const SubscriptionHandler& handler) {

    const auto topic = utils::get_variable_topic(other_peer_id, implementation_id, variable_name);
    const auto subscription_key = get_subscription_key(other_peer_id, implementation_id);

    // key other_peer/impl/var -> topic impl_name/

    auto& worker_handle = this->json_handlers.get(subscription_key);

    auto worker = worker_handle.get_locked_access();

    auto handler_info = worker.add_handler(variable_name, handler);

    const auto handler_list_was_empty = handler_info.first;
    if (handler_list_was_empty) {
        // FIXME (aw): which QOS do we need here
        this->mqtt_io.subscribe(topic, mqtt::QOS::QOS2);
    }

    return [&mqtt_io = this->mqtt_io, topic, variable_name, &worker_handle, handler_it = handler_info.second]() {
        auto worker = worker_handle.get_locked_access();
        const auto handler_list_is_now_empty = worker.remove_handler(variable_name, handler_it);
        if (handler_list_is_now_empty) {
            mqtt_io.unsubscribe(topic);
        }
    };
}

void Peer::implement_command(const std::string& implementation_id, const std::string& command_name,
                             const CommandHandler& handler) {

    const auto command_key = get_command_key(implementation_id);

    auto& worker_handle = this->json_handlers.get(command_key);

    auto worker = worker_handle.get_locked_access();

    if (worker.handler_count(command_name) != 0) {
        // command already implemented, this is not allowed
        // FIXME (aw): exception handling
        throw std::runtime_error(fmt::format("Tried to implement command '{}' again", command_name));
    }

    const auto topic = utils::get_command_topic(this->peer_id, implementation_id, command_name);

    // NOTE (aw): we need QOS2, because otherwise the command could be issued twice
    this->mqtt_io.subscribe(topic, mqtt::QOS::QOS2);

    auto handler_wrapper = [&mqtt_io = this->mqtt_io, handler](const nlohmann::json& msg) {
        // msg has been checked for frame format already in handle_command

        auto result = handler(msg.value("params", nlohmann::json(nullptr)));

        const auto call_return_message = nlohmann::json({
            {"id", msg.at("id")},
            {"result", std::move(result)},
        });

        const auto other_peer_result_topic = utils::get_result_topic(msg.at("peer"));

        // NOTE (aw): QOS1 is enough here, because we check if the promise has been set already
        mqtt_io.publish(other_peer_result_topic, call_return_message.dump(), mqtt::QOS::QOS1);
    };

    auto handler_info = worker.add_handler(command_name, handler_wrapper);
}

UnsubscriptionCallback Peer::mqtt_subscribe(const std::string& topic, const MQTTSubscriptionHandler& handler) {
    // FIXME (aw): we should specialize for this case, because there is
    // only one key for the worker for string_handlers
    // FIXME (aw): string literal ...
    auto& worker_handle = this->string_handlers.get("mqtt");

    auto worker = worker_handle.get_locked_access();

    auto handler_info = worker.add_handler(topic, handler);

    const auto handler_list_was_empty = handler_info.first;
    if (handler_list_was_empty) {
        // FIXME (aw): which QOS do we need here
        this->mqtt_io.subscribe(topic, mqtt::QOS::QOS2);
    }

    return [&mqtt_io = this->mqtt_io, topic, &worker_handle, handler_it = handler_info.second]() {
        auto worker = worker_handle.get_locked_access();
        const auto handler_list_is_now_empty = worker.remove_handler(topic, handler_it);
        if (handler_list_is_now_empty) {
            mqtt_io.unsubscribe(topic);
        }
    };
}

void Peer::mqtt_publish(const std::string& topic, const std::string& data) {
    this->mqtt_io.publish(topic, data, mqtt::QOS::QOS2);
}

void Peer::handle_subscription(nlohmann::json message, const utils::TopicInfo& topic_info) {
    const std::string topic_key = get_subscription_key(topic_info.peer_id, topic_info.impl_id);
    auto worker = this->json_handlers.find(topic_key);

    if (!worker) {
        // FIXME (aw): exception handling
        throw std::runtime_error("Received on a subscription topic, which we never subscribed");
    }

    worker->get_locked_access().add_work(topic_info.name, std::move(message));
}

void Peer::handle_command(nlohmann::json message, const utils::TopicInfo& topic_info) {
    if (!message.is_object() || !message.contains("peer") || !message.contains("id")) {
        // FIXME (aw): we need a separate message frame check function, which also checks the types of the keys and if
        // for example peer doesn't contain slashes etc
        EVLOG(warning) << "Received invalid call message";
        return;
    }

    const auto topic_key = get_command_key(topic_info.impl_id);

    auto worker = this->json_handlers.find(topic_key);

    if (!worker) {
        // FIXME (aw): exception handling
        throw std::runtime_error("Received on a command topic, which we never subscribed");
    }

    worker->get_locked_access().add_work(topic_info.name, std::move(message));
}

void Peer::handle_result(nlohmann::json message) {
    if (!message.is_object() || !message.contains("id")) {
        // FIXME (aw): we need a separate message frame check function, which also checks the types of the keys and if
        // for example peer doesn't contain slashes etc
        EVLOG(warning) << "Received invalid result message";
        return;
    }

    const CallIDType call_id = message.at("id");

    // FIXME (aw): we could also try to move here
    const auto result = (message.value("result", nlohmann::json(nullptr)));

    // FIXME (aw): check if command successful, otherwise log
    const auto result_set_successful = this->calls.set_result(call_id, std::move(result));

    if (!result_set_successful) {
        EVLOG(warning) << fmt::format("Invalid call id '{}' was referenced on peer '{}'", call_id, this->peer_id);
    }
}

void Peer::handle_external_mqtt(const std::string& topic, std::string message) {
    auto worker = this->string_handlers.find("mqtt");

    if (!worker) {
        // FIXME (aw): exception handling
        throw std::runtime_error("Received on a subscription topic, which we never subscribed");
    }

    worker->get_locked_access().add_work(topic, std::move(message));
}

Peer::ExecutedCalls::ExecutedCallMap::iterator Peer::ExecutedCalls::get(std::mt19937& rng) {
    const std::lock_guard<std::mutex> lock(this->mutex);
    auto new_call = this->map.emplace(rng(), std::promise<Value>());
    while (!new_call.second) {
        // call existed already (very unlikely), trying next one
        new_call = this->map.emplace(rng(), std::promise<Value>());
    }

    return new_call.first;
}

void Peer::ExecutedCalls::release(const ExecutedCallMap::iterator& it) {
    const std::lock_guard<std::mutex> lock(this->mutex);
    this->map.erase(it);
    // not really clear, what should be done here ...
}

bool Peer::ExecutedCalls::set_result(CallIDType id, Value result) {
    const std::lock_guard<std::mutex> lock(this->mutex);
    auto call = this->map.find(id);

    if (call == this->map.end()) {
        return false;
    }

    try {
        call->second.set_value(std::move(result));
        return true;
    } catch (const std::future_error& e) {
        if (e.code() == std::future_errc::promise_already_satisfied) {
            // this can happen if we multiply get results back due to QOS1
            return false;
        }
        throw;
    }
}

} // namespace everest
