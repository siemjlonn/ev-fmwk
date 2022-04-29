// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_UTILS_HELPERS_HPP
#define EVEREST_UTILS_HELPERS_HPP

#include <string>

namespace everest::utils {
struct TopicInfo {
    enum Type
    {
        OTHER,
        VAR,
        CMD,
        RESULT,
        INVALID
    } type;
    std::string peer_id;
    std::string impl_id;
    std::string name;
};

TopicInfo parse_topic(const std::string& topic);

std::string get_variable_topic(const std::string& peer_id, const std::string& impl_id, const std::string& var);
std::string get_command_topic(const std::string& peer_id, const std::string& impl_id, const std::string& cmd);
std::string get_result_topic(const std::string& peer_id);

std::string md5_hash(const void* data, size_t len);
} // namespace everest::utils

#endif // EVEREST_UTILS_HELPERS_HPP
