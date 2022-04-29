// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_CONSTANTS_HPP
#define EVEREST_CONSTANTS_HPP

#include <cstdint>

namespace everest::defaults {

constexpr uint16_t MQTT_PORT = 1883;
constexpr auto MQTT_LOCATION = "mqtt-server";
constexpr auto ENV_VAR_MQTT_PORT = "MQTT_SERVER_PORT";
// FIXME (aw): MQTT_SERVER_ADDRESS should be named MQTT_SERVER_LOCATION
constexpr auto ENV_VAR_MQTT_LOCATION = "MQTT_SERVER_ADDRESS";
constexpr auto MQTT_TOPIC_EVEREST_LITERAL = "everest";
constexpr auto MQTT_TOPIC_RESULT_LITERAL = "result";
constexpr auto MQTT_TOPIC_VAR_LITERAL = "var";
constexpr auto MQTT_TOPIC_CMD_LITERAL = "cmd";

constexpr auto CALL_TIMEOUT_MS = 3000;

} // namespace everest::defaults

#endif // EVEREST_CONSTANTS_HPP
