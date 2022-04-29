// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef TESTS_TEST_HELPERS_HPP
#define TESTS_TEST_HELPERS_HPP

#include <condition_variable>
#include <mutex>

#include <nlohmann/json.hpp>

#include <everest/utils/mqtt_io_base.hpp>

namespace everest::mqtt {

class IOBaseMock : public IOBase {
public:
    struct PublishedMessage {
        std::string topic;
        std::string payload;
        QOS qos;
    };

    void inject_message(const std::string& topic, const nlohmann::json& message);

    std::string last_subscription_topic;
    QOS last_subscription_qos;
    std::string last_unsubscription_topic;
    PublishedMessage last_published_message{"", "reset", everest::mqtt::QOS::QOS0};

private:
    void subscribe(const std::string& topic, QOS qos) override final;
    void unsubscribe(const std::string& topic) override final;
    void publish(const std::string& topic, const std::string& data, QOS qos) override final;
    bool sync(int timeout_ms) override final;
};

} // namespace everest::mqtt

class MessageSync {
public:
    void push_message(const nlohmann::json& msg);
    nlohmann::json pop_message(int timeout_ms = 20);

private:
    std::mutex mutex;
    std::condition_variable cv_can_push;
    std::condition_variable cv_can_pop;
    nlohmann::json message;
    bool has_message{false};
};

std::string read_resource_file(const std::string& path);
nlohmann::json read_json_file(const std::string& path);

#endif // TESTS_TEST_HELPERS_HPP
