// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef SRC_MANAGER_RUNTIME_ENV_HPP
#define SRC_MANAGER_RUNTIME_ENV_HPP

#include <boost/filesystem.hpp>

namespace defaults {
constexpr auto RUNTIME_DIR = "/usr/lib/everest";
constexpr auto CONFIG_FILE = "config.json";
constexpr auto LOGGING_CONFIG_FILE = "logging.conf";
constexpr auto MODULE_SUB_DIR = "modules";
} // namespace defaults

boost::filesystem::path resolve_path_if_exists(const boost::filesystem::path& path);

struct RuntimeEnvironment {
    boost::filesystem::path runtime_path;
    std::string config;
    boost::filesystem::path modules_path;
    boost::filesystem::path logging_config_path;
    bool only_validate{false};
};

struct RuntimeEnvironmentException : public std::runtime_error {
    RuntimeEnvironmentException(const std::string& message, bool show_usage = false) :
        std::runtime_error(message), show_usage(show_usage){};
    bool show_usage{false};
};

RuntimeEnvironment handle_command_line_args(int argc, char* const argv[]);

#endif // SRC_MANAGER_RUNTIME_SETTINGS_HPP
