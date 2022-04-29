// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <everest/generated_module.hpp>

namespace everest {

ExternalMQTT::ExternalMQTT(ModulePeer& module_peer) : module_peer(module_peer) {
}

UnsubscriptionCallback ExternalMQTT::subscribe(const std::string& topic, const MQTTSubscriptionHandler& handler) {
    return this->module_peer.mqtt_subscribe_variable(topic, handler);
}

void ExternalMQTT::publish(const std::string& topic, const std::string& data) {
    this->module_peer.mqtt_publish_variable(topic, data);
}

} // namespace everest
