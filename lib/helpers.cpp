// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <everest/utils/helpers.hpp>

#include <fmt/core.h>

#include <hash_library/md5.h>

#include <everest/constants.hpp>

namespace everest::utils {

inline std::string get_topic(const std::string& peer_id, const std::string& impl_id, const std::string& type,
                             const std::string& name) {
    if (impl_id.empty()) {
        return fmt::format("{}/{}/{}/{}", defaults::MQTT_TOPIC_EVEREST_LITERAL, peer_id, type, name);
    } else {
        return fmt::format("{}/{}/{}/{}/{}", defaults::MQTT_TOPIC_EVEREST_LITERAL, peer_id, impl_id, type, name);
    }
}

std::string get_variable_topic(const std::string& peer_id, const std::string& impl_id, const std::string& var) {
    return get_topic(peer_id, impl_id, defaults::MQTT_TOPIC_VAR_LITERAL, var);
}

std::string get_command_topic(const std::string& peer_id, const std::string& impl_id, const std::string& cmd) {
    return get_topic(peer_id, impl_id, defaults::MQTT_TOPIC_CMD_LITERAL, cmd);
}

std::string get_result_topic(const std::string& peer_id) {
    return fmt::format("{}/{}/{}", defaults::MQTT_TOPIC_EVEREST_LITERAL, peer_id, defaults::MQTT_TOPIC_RESULT_LITERAL);
}

static void parse_topic_type(const std::string& topic, size_t offset, size_t separator_pos, TopicInfo& topic_info) {
    using namespace everest::defaults;
    constexpr auto var_literal_len = strlen(MQTT_TOPIC_VAR_LITERAL);
    constexpr auto cmd_literal_len = strlen(MQTT_TOPIC_CMD_LITERAL);

    if ((separator_pos - offset == var_literal_len) &&
        (topic.compare(offset, var_literal_len, MQTT_TOPIC_VAR_LITERAL) == 0)) {
        topic_info.type = TopicInfo::VAR;
        topic_info.name = topic.substr(separator_pos + 1);
    }

    if ((separator_pos - offset == cmd_literal_len) &&
        (topic.compare(offset, cmd_literal_len, MQTT_TOPIC_CMD_LITERAL) == 0)) {
        topic_info.type = TopicInfo::CMD;
        topic_info.name = topic.substr(separator_pos + 1);
    }
}

TopicInfo parse_topic(const std::string& topic) {
    using namespace everest::defaults;
    constexpr auto everest_literal_len = strlen(MQTT_TOPIC_EVEREST_LITERAL);

    TopicInfo topic_info = {TopicInfo::INVALID, "", "", ""};

    // try to check the most likely paths

    if (topic.compare(0, everest_literal_len, MQTT_TOPIC_EVEREST_LITERAL) != 0) {
        topic_info.type = TopicInfo::OTHER;
        return topic_info;
    }

    if (topic.find('/', everest_literal_len) != everest_literal_len) {
        // "everest" is not followed by a '/'
        if (topic.length() == everest_literal_len) {
            // if it is only everest, we consider this as invalid
            return topic_info;
        } else {
            // otherwise its something else
            topic_info.type = TopicInfo::OTHER;
            return topic_info;
        }
    };

    // we definitely have "everest/"
    const auto module_first_char_pos = everest_literal_len + 1;
    const auto second_slash_pos = topic.find('/', module_first_char_pos);
    if (second_slash_pos == std::string::npos) {
        // no second slash -> invalid
        return topic_info;
    }

    const auto peer_id_len = second_slash_pos - module_first_char_pos;
    if (peer_id_len == 0) {
        // invalid
        return topic_info;
    }

    topic_info.peer_id = topic.substr(module_first_char_pos, peer_id_len);

    // match1 should be either result, impl_id or cmd/var
    const auto match1_first_pos = second_slash_pos + 1;
    const auto third_slash_pos = topic.find('/', match1_first_pos);
    if (third_slash_pos == std::string::npos) {
        // could be result
        constexpr auto result_literal_len = strlen(MQTT_TOPIC_RESULT_LITERAL);
        if (topic.compare(match1_first_pos, std::string::npos, MQTT_TOPIC_RESULT_LITERAL) == 0) {
            topic_info.type = TopicInfo::RESULT;
            return topic_info;
        }
        // otherwise invalid
        return topic_info;
    }

    const auto match1_len = third_slash_pos - match1_first_pos;

    if (match1_len == 0) {
        // invalid
        return topic_info;
    }

    // match2 should be either impl_id or or cmd/var
    const auto match2_first_pos = third_slash_pos + 1;
    const auto fourth_slash_pos = topic.find('/', match2_first_pos);

    if (fourth_slash_pos == std::string::npos) {
        const auto match2_len = topic.length() - (third_slash_pos + 1);
        if (match2_len == 0) {
            // invalid
            return topic_info;
        }
        // we should have no impl_id
        parse_topic_type(topic, match1_first_pos, third_slash_pos, topic_info);
        return topic_info;
    }

    const auto match2_len = fourth_slash_pos - match2_first_pos;

    if (match2_len == 0) {
        // invalid
        return topic_info;
    }

    const auto match3_first_pos = fourth_slash_pos + 1;

    if (match3_first_pos > topic.length() - 1) {
        // invalid
        return topic_info;
    }

    if (topic.find('/', fourth_slash_pos + 1) != std::string::npos) {
        // additional '/' -> invalid
        return topic_info;
    }

    topic_info.impl_id = topic.substr(match1_first_pos, match1_len);

    parse_topic_type(topic, match2_first_pos, fourth_slash_pos, topic_info);
    return topic_info;
}

std::string md5_hash(const void* data, size_t len) {
    MD5 md5_hasher;
    return md5_hasher(data, len);
}

} // namespace everest::utils
