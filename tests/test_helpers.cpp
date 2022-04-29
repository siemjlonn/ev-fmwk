// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <catch2/catch.hpp>

#include <everest/utils/helpers.hpp>

SCENARIO("Check helper parsing functions", ) {
    WHEN("We want to test utils::parse_topic") {
        using namespace everest::utils;

        THEN("Non everest topics should yield an OTHER topic type") {
            REQUIRE(parse_topic("$sys").type == TopicInfo::OTHER);
            REQUIRE(parse_topic("$sys/bla").type == TopicInfo::OTHER);
            REQUIRE(parse_topic("evErest/dk").type == TopicInfo::OTHER);
            REQUIRE(parse_topic("everest_hi").type == TopicInfo::OTHER);
        }

        THEN("Invalid everest topics should yield INVALID topic type") {
            REQUIRE(parse_topic("everest").type == TopicInfo::INVALID);
            REQUIRE(parse_topic("everest/foo").type == TopicInfo::INVALID);
            REQUIRE(parse_topic("everest/").type == TopicInfo::INVALID);
            REQUIRE(parse_topic("everest//").type == TopicInfo::INVALID);
            REQUIRE(parse_topic("everest/module//").type == TopicInfo::INVALID);
            REQUIRE(parse_topic("everest/module/resulte").type == TopicInfo::INVALID);
            REQUIRE(parse_topic("everest/module//d").type == TopicInfo::INVALID);
            REQUIRE(parse_topic("everest/module/d/").type == TopicInfo::INVALID);
            REQUIRE(parse_topic("everest/module/d//").type == TopicInfo::INVALID);
            REQUIRE(parse_topic("everest/module/d/s/").type == TopicInfo::INVALID);

            REQUIRE(parse_topic("everest/module/d/vars//").type == TopicInfo::INVALID);
        }

        THEN("Valid result topic should yield RESULT topic type") {
            (parse_topic("everest/peer_id/result").type == TopicInfo::RESULT);
        }

        THEN("Everest cmd topics should yield CMD topic type") {
            auto topic_info = parse_topic("everest/peer_id/cmd/blabal");
            REQUIRE(topic_info.type == TopicInfo::CMD);
            REQUIRE(topic_info.name == "blabal");
            REQUIRE(topic_info.peer_id == "peer_id");

            topic_info = parse_topic("everest/peer_id/impl_id/cmd/cmx");
            REQUIRE(topic_info.type == TopicInfo::CMD);
            REQUIRE(topic_info.name == "cmx");
            REQUIRE(topic_info.impl_id == "impl_id");
            REQUIRE(topic_info.peer_id == "peer_id");
        }

        THEN("Everest var topics should yield VAR topic type") {
            auto topic_info = parse_topic("everest/peer_id/var/blabal");
            REQUIRE(topic_info.type == TopicInfo::VAR);
            REQUIRE(topic_info.name == "blabal");
            REQUIRE(topic_info.peer_id == "peer_id");

            topic_info = parse_topic("everest/peer_id/impl_id/var/var1");
            REQUIRE(topic_info.type == TopicInfo::VAR);
            REQUIRE(topic_info.name == "var1");
            REQUIRE(topic_info.impl_id == "impl_id");
            REQUIRE(topic_info.peer_id == "peer_id");
        }
    }
}

SCENARIO("Test the md5 hasher") {
    GIVEN("Some sample data and it known md5 hash") {
        const std::string sample_data{"Hello World"};
        const std::string known_md5_sum{"b10a8db164e0754105b7a99be72e3fe5"};

        REQUIRE(everest::utils::md5_hash(sample_data.c_str(), sample_data.length()) == known_md5_sum);
    }
}
