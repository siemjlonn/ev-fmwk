// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <catch2/catch.hpp>

#include <everest/utils/message_worker.hpp>

#include "testing_helpers.hpp"

SCENARIO("Check message worker functionality", ) {
    GIVEN("A message worker instance and locked access to it") {
        everest::utils::MessageWorker<nlohmann::json> mw;

        MessageSync syncer;
        auto handler = [&syncer](const nlohmann::json& data) { syncer.push_message(data); };

        WHEN("Adding a new handler") {
            const auto topic_id = "topic_id";
            auto handler_info = mw.get_locked_access().add_handler(topic_id, handler);

            REQUIRE(handler_info.first == true);

            THEN("Adding a next handler should not yield an initially list") {
                auto mw_access = mw.get_locked_access();
                handler_info = mw_access.add_handler(topic_id, handler);

                REQUIRE(handler_info.first == false);

                AND_THEN("Removing the second handler should return a non-empty list") {
                    auto is_empty = mw_access.remove_handler(topic_id, handler_info.second);
                    REQUIRE(is_empty == false);
                }
            }

            THEN("Removing that handler should yield an empty list") {
                auto is_empty = mw.get_locked_access().remove_handler(topic_id, handler_info.second);
                REQUIRE(is_empty == true);

                AND_THEN("Adding work to the worker should not yield any messages") {
                    mw.get_locked_access().add_work(topic_id, nlohmann::json("verify_me"));

                    REQUIRE(syncer.pop_message() == "timeout");
                }
            }

            THEN("Adding some data should call the handler once") {
                mw.get_locked_access().add_work(topic_id, nlohmann::json("verify_me"));

                REQUIRE(syncer.pop_message() == "verify_me");
                REQUIRE(syncer.pop_message() == "timeout");
            }

            THEN("Submitting many messages should yield exactly that much messages and in correct order") {
                const auto count = 10;
                {
                    auto mw_access = mw.get_locked_access();
                    for (int i = 0; i < count; ++i) {
                        const auto payload = "verify_me" + std::to_string(i);
                        mw_access.add_work(topic_id, nlohmann::json(payload));
                    }
                }

                THEN("Exactly that much message should come back") {
                    for (int i = 0; i < count; ++i) {
                        auto payload = syncer.pop_message();
                        REQUIRE(("verify_me" + std::to_string(i)) == payload);
                    }

                    REQUIRE(syncer.pop_message() == "timeout");
                }
            }

            AND_WHEN("Adding a second handler") {
                mw.get_locked_access().add_handler(topic_id, handler);

                THEN("Pushing a message should call both handlers") {
                    auto data = nlohmann::json("verify_me");
                    mw.get_locked_access().add_work(topic_id, std::move(data));

                    REQUIRE(syncer.pop_message() == "verify_me");
                    REQUIRE(syncer.pop_message() == "verify_me");
                    REQUIRE(syncer.pop_message() == "timeout");
                }
            }
        }
    }
}
