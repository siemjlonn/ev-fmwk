// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_UTILS_MQTT_IO_BASE_HPP
#define EVEREST_UTILS_MQTT_IO_BASE_HPP

#include <string>
#include <functional>

namespace everest::mqtt {

/// \brief MQTT Quality of service
enum class QOS
{
    QOS0, ///< At most once delivery
    QOS1, ///< At least once delivery
    QOS2  ///< Exactly once delivery
};

struct ServerAddress {
    std::string location;
    uint16_t port;
};

struct RawMessage {
    const char* topic;
    uint16_t topic_len;
    const char* payload;
    size_t payload_len;
};

// FIXME (aw): if we have the handler right, we might not even need the message queue object here
class IOBase {
public:
    IOBase();
    IOBase(const std::function<void(const RawMessage&)>& message_handler);

    virtual ~IOBase() = default;

    virtual void subscribe(const std::string& topic, QOS qos) = 0;
    virtual void unsubscribe(const std::string& topic) = 0;

    virtual void publish(const std::string& topic, const std::string& data, QOS qos) = 0;

    // FIXME (aw): this is probably a bit to much to expose
    void set_handler(const std::function<void(const RawMessage&)>& message_handler);


    // sync buffers
    virtual bool sync(int timeout_ms) = 0;

protected:
    std::function<void(const RawMessage&)> handler;
};

} // namespace everest::mqtt

#endif // EVEREST_UTILS_MQTT_IO_BASE_HPP
