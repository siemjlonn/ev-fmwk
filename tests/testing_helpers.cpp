// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include "testing_helpers.hpp"

#include <boost/filesystem/string_file.hpp>

namespace everest::mqtt {

void IOBaseMock::subscribe(const std::string& topic, QOS qos) {
    last_subscription_topic = topic;
    last_subscription_qos = qos;
}

void IOBaseMock::unsubscribe(const std::string& topic) {
    last_unsubscription_topic = topic;
}

void IOBaseMock::publish(const std::string& topic, const std::string& data, QOS qos) {
    last_published_message = {topic, data, qos};
}

bool IOBaseMock::sync(int timeout_ms) {
    return true;
    // todo
}

void IOBaseMock::inject_message(const std::string& topic, const nlohmann::json& message) {
    const auto message_string = message.dump();
    auto raw_message = everest::mqtt::RawMessage{
        topic.data(),
        static_cast<uint16_t>(topic.length()),
        message_string.data(),
        message_string.length(),
    };
    this->handler(raw_message);
}

} // namespace everest::mqtt

void MessageSync::push_message(const nlohmann::json& msg) {
    std::unique_lock<std::mutex> lock(mutex);
    while (has_message) {
        cv_can_push.wait(lock);
    }

    message = msg;
    has_message = true;
    cv_can_pop.notify_one();
}

nlohmann::json MessageSync::pop_message(int timeout_ms) {
    std::unique_lock<std::mutex> lock(mutex);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (!has_message) {
        if (std::cv_status::timeout == cv_can_pop.wait_until(lock, deadline)) {
            return "timeout";
        }
    }

    has_message = false;
    cv_can_push.notify_one();
    return message;
}

std::string read_resource_file(const std::string& path) {
    std::string data;

    const auto fs_path = boost::filesystem::path(TEST_RESOURCES_DIRECTORY) / "resources" / path;
    boost::filesystem::load_string_file(fs_path, data);

    return data;
}

nlohmann::json read_json_file(const std::string& path) {
    const auto data = read_resource_file(path);
    return nlohmann::json::parse(data);
}
