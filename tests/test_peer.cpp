// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <catch2/catch.hpp>

#include "testing_helpers.hpp"

#include <nlohmann/json.hpp>

#include <everest/peer.hpp>

SCENARIO("Check mqtt abstraction layer") {
    everest::mqtt::IOBaseMock mqtt_io;

    MessageSync syncer;
    auto primitive_callback = [&syncer](const nlohmann::json& payload) { syncer.push_message(payload); };

    GIVEN("A mocked mqtt io handle and an fresh mqtt abstraction instance ") {

        everest::Peer peer("peer", mqtt_io);

        THEN("No new message should be outgoing") {
            REQUIRE(mqtt_io.last_published_message.payload == "reset");
        }
    }

    GIVEN("A connected mqtt abstraction instance") {

        everest::Peer peer("peer", mqtt_io);

        WHEN("Publishing a message ") {
            peer.publish_variable("topic", "", std::string("data"));

            THEN("The default QOS level on publish should be") {
                REQUIRE(mqtt_io.last_published_message.qos == everest::mqtt::QOS::QOS2);
            }
        }

        WHEN("Publishing a message") {
            const std::string variable = "somewhere";
            const std::string payload = "some data";

            peer.publish_variable("", variable, payload);

            THEN("The same message parts should arrive at the MQTT IO") {
                REQUIRE(mqtt_io.last_published_message.topic == "everest/peer/var/somewhere");
                REQUIRE(mqtt_io.last_published_message.payload == "\"some data\"");
            }
        }

        WHEN("Subscribing to a variable") {
            auto unsubscribe = peer.subscribe_variable("other_peer", "", "var_name", primitive_callback);

            THEN("The correct request should be forwarded to MQTT IO") {
                REQUIRE(mqtt_io.last_subscription_topic == "everest/other_peer/var/var_name");
                REQUIRE(mqtt_io.last_subscription_qos == everest::mqtt::QOS::QOS2);
            }

            AND_WHEN("MQTT IO yields some data") {
                nlohmann::json data = "verify_me";
                mqtt_io.inject_message("everest/other_peer/var/var_name", data);

                THEN("The corresponding data should arrive at the handler") {
                    REQUIRE(syncer.pop_message() == "verify_me");
                }
            }
        }
    }
}
