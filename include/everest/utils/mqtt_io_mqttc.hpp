#ifndef EVEREST_UTILS_MQTT_IO_MQTTC_HPP
#define EVEREST_UTILS_MQTT_IO_MQTTC_HPP

#include <mqtt.h>

#include "mqtt_io_base.hpp"

const int BUF_SIZE = 150 * 1024; // 150K
const int SYNC_SLEEP_MS = 10;
const int KEEP_ALIVE_S = 400;

namespace everest::mqtt {

ServerAddress get_default_server_address();

class MQTTC_IO : public IOBase {
public:
    MQTTC_IO(const ServerAddress& server_address);
    ~MQTTC_IO();

    void subscribe(const std::string& topic, QOS qos) override final;
    void unsubscribe(const std::string& topic) override final;

    void publish(const std::string& topic, const std::string& data, QOS qos) override final;

    bool sync(int timeout_ms) override final;

    const ServerAddress server_address;

private:
    bool connect();
    void disconnect();

    void notify_write_data();
    int mqtt_socket_fd{-1};
    int event_fd{-1};
    struct mqtt_client mqtt_client {};
    uint8_t sendbuf[BUF_SIZE];
    uint8_t recvbuf[BUF_SIZE];
};

} // namespace everest::mqtt

#endif // EVEREST_UTILS_MQTT_IO_MQTTC_HPP
