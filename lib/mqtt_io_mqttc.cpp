// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <everest/utils/mqtt_io_mqttc.hpp>

#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fmt/core.h>

#include <everest/constants.hpp>
#include <everest/logging/logging.hpp>
#include <everest/utils/os.hpp>

static int open_non_blocking_socket(const everest::mqtt::ServerAddress& addr) {
    // open the non-blocking TCP socket (connecting to the broker)

    struct addrinfo hints = {0, 0, 0, 0, 0, 0, 0, 0};

    hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Must be TCP
    int sockfd = -1;
    int rv = 0;
    struct addrinfo* p = nullptr;
    struct addrinfo* servinfo = nullptr;

    // get address information
    rv = getaddrinfo(addr.location.c_str(), std::to_string(addr.port).c_str(), &hints, &servinfo);
    if (rv != 0) {
        // fprintf(stderr, "Failed to open socket (getaddrinfo): %s\n",
        // gai_strerror(rv));
        return -1;
    }

    // open the first possible socket
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            continue;
        }

        // connect to server
        rv = ::connect(sockfd, p->ai_addr, p->ai_addrlen);
        if (rv == -1) {
            close(sockfd);
            sockfd = -1;
            continue;
        }
        break;
    }

    // free servinfo
    freeaddrinfo(servinfo);

    // make non-blocking
    if (sockfd != -1) {
        fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK); // NOLINT: We have no good alternative to fcntl
    }

    // return the new socket fd
    return sockfd;
}

void mqtt_publish_callback(void** state, struct mqtt_response_publish* published) {
    auto message_handler = static_cast<std::function<void(const everest::mqtt::RawMessage&)>*>(*state);

    message_handler->operator()({static_cast<const char*>(published->topic_name), published->topic_name_size,
                                 static_cast<const char*>(published->application_message),
                                 published->application_message_size});
}

namespace everest::mqtt {

ServerAddress get_default_server_address() {
    mqtt::ServerAddress address{defaults::MQTT_LOCATION, defaults::MQTT_PORT};

    const auto location_from_env = os::get_environment_variable(defaults::ENV_VAR_MQTT_LOCATION);
    if (!location_from_env.empty()) {
        address.location = location_from_env;
    }

    const auto port_from_env = os::get_environment_variable(defaults::ENV_VAR_MQTT_PORT);
    if (!port_from_env.empty()) {
        try {
            auto port = std::stoi(port_from_env);
            using port_limits = std::numeric_limits<decltype(defaults::MQTT_PORT)>;
            if (port >= port_limits::min() && port <= port_limits::max()) {
                address.port = port;
            }
        } catch (const std::exception&) {
            // nop
        }
    }

    return address;
}

static inline int max_qos_level(QOS qos) {
    switch (qos) {
    case QOS::QOS0:
        return 0;
    case QOS::QOS1:
        return 1;
    case QOS::QOS2:
        return 2;
    default:
        return 0;
    }
}

static inline int publish_flags(QOS qos, bool retain = false) {
    switch (qos) {
    case QOS::QOS0:
        return (retain) ? MQTT_PUBLISH_QOS_0 & MQTT_PUBLISH_RETAIN : MQTT_PUBLISH_QOS_0;
    case QOS::QOS1:
        return (retain) ? MQTT_PUBLISH_QOS_1 & MQTT_PUBLISH_RETAIN : MQTT_PUBLISH_QOS_1;
    case QOS::QOS2:
        return (retain) ? MQTT_PUBLISH_QOS_2 & MQTT_PUBLISH_RETAIN : MQTT_PUBLISH_QOS_2;
    default:
        return (retain) ? MQTT_PUBLISH_QOS_0 & MQTT_PUBLISH_RETAIN : MQTT_PUBLISH_QOS_0;
    }
}

MQTTC_IO::MQTTC_IO(const ServerAddress& server_address) : IOBase(), server_address(server_address) {
    this->mqtt_client.publish_response_callback_state = &this->handler;
    if (!this->connect()) {
        // FIXME (aw): exception handling
        throw std::runtime_error(
            fmt::format("Could not connect via MQTT to {}:{}\n", server_address.location, server_address.port));
    }
}

MQTTC_IO::~MQTTC_IO() {
    this->disconnect();
}

bool MQTTC_IO::connect() {
    EVLOG(info) << fmt::format("Connecting to MQTT broker: {}:{}", this->server_address.location,
                               this->server_address.port);
    this->mqtt_socket_fd = open_non_blocking_socket(this->server_address);

    if (this->mqtt_socket_fd == -1) {
        perror("Failed to open socket: ");
        return false;
    }

    this->event_fd = eventfd(0, 0);
    if (this->event_fd == -1) {
        close(this->mqtt_socket_fd);
        EVLOG(error) << "Could not setup eventfd for mqttc io";
        return false;
    }

    mqtt_init(&this->mqtt_client, this->mqtt_socket_fd, static_cast<uint8_t*>(this->sendbuf), sizeof(this->sendbuf),
              static_cast<uint8_t*>(this->recvbuf), sizeof(this->recvbuf), mqtt_publish_callback);
    uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
    /* Send connection request to the broker. */
    if (mqtt_connect(&this->mqtt_client, nullptr, nullptr, nullptr, 0, nullptr, nullptr, connect_flags, KEEP_ALIVE_S) ==
        MQTT_OK) {
        mqtt_sync(&this->mqtt_client);
        // TODO(kai): async?
        return true;
    }

    return false;
}

bool MQTTC_IO::sync(int timeout_ms) {
    // FIXME (aw): error handling
    eventfd_t eventfd_buffer;
    struct pollfd pollfds[2] = {{this->mqtt_socket_fd, POLLIN, 0}, {this->event_fd, POLLIN, 0}};
    auto retval = ::poll(pollfds, 2, timeout_ms);

    if (retval == 0) {
        return true;
    } else if (retval > 0) {
        // check for write notification and reset it
        if (pollfds[1].revents & POLLIN) {
            // FIXME (aw): check for failure
            eventfd_read(this->event_fd, &eventfd_buffer);
        }

        // we either received an event or there is new data on the socket, in both cases we want to sync
        auto error = mqtt_sync(&this->mqtt_client);
        if (MQTT_OK == error) {
            return true;
        } else {
            EVLOG(error) << fmt::format("MQTT ERROR: {}", mqtt_error_str(error));
        }
    }

    return false;
}

void MQTTC_IO::disconnect() {
    // FIXME (aw): check if we connected
    if (this->mqtt_socket_fd != -1) {
        mqtt_disconnect(&this->mqtt_client);
        mqtt_sync(&this->mqtt_client);
        close(this->mqtt_socket_fd);
    }

    if (this->event_fd != -1) {
        close(this->event_fd);
    }
}

void MQTTC_IO::publish(const std::string& topic, const std::string& data, QOS qos) {

    MQTTErrors error = mqtt_publish(&this->mqtt_client, topic.c_str(), data.c_str(), data.size(), publish_flags(qos));
    if (error != MQTT_OK) {
        EVLOG(error) << fmt::format("MQTT Error {}", mqtt_error_str(error));
    }

    notify_write_data();
}

void MQTTC_IO::subscribe(const std::string& topic, QOS qos) {
    EVLOG(debug) << fmt::format("Subscribing to topic: {}", topic);
    mqtt_subscribe(&this->mqtt_client, topic.c_str(), max_qos_level(qos));
    notify_write_data();
}

void MQTTC_IO::unsubscribe(const std::string& topic) {
    EVLOG(debug) << fmt::format("Unsubscribing from topic: {}", topic);
    mqtt_unsubscribe(&this->mqtt_client, topic.c_str());
    notify_write_data();
}

void MQTTC_IO::notify_write_data() {
    // FIXME (aw): error handling
    eventfd_write(this->event_fd, 1);
}

} // namespace everest::mqtt
