// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest

#include "runtime_env.hpp"

#include <sstream>

#include <boost/program_options.hpp>

#include <fmt/format.h>

namespace fs = boost::filesystem;

template <typename T>
T get_option_or_default(const boost::program_options::variables_map& vm, const std::string& name, T default_value) {
    return vm.count(name) ? vm[name].as<T>() : default_value;
}

static inline auto get_option_or_default(const boost::program_options::variables_map& vm, const std::string& name,
                                         const fs::path& default_value) {
    return get_option_or_default(vm, name, default_value.string());
}

fs::path resolve_path_if_exists(const fs::path& path) {
    return fs::exists(path) ? fs::canonical(path) : path;
}

RuntimeEnvironment handle_command_line_args(int argc, char* const argv[]) {
    namespace po = boost::program_options;

    const auto DEFAULT_CONFIG_FILE_PATH = fmt::format("{}/{}", defaults::RUNTIME_DIR, defaults::CONFIG_FILE);
    const auto DEFAULT_LOGGING_CONFIG_FILE_PATH =
        fmt::format("{}/{}", defaults::RUNTIME_DIR, defaults::LOGGING_CONFIG_FILE);
    const auto DEFAULT_MODULE_DIR = fmt::format("{}/{}", defaults::RUNTIME_DIR, defaults::MODULE_SUB_DIR);

    RuntimeEnvironment rs;

    po::options_description desc("EVerest manager");
    desc.add_options()("help,h", "This help message");
    desc.add_options()("validate", po::bool_switch(&rs.only_validate), "Only parse and validate the config file");
    desc.add_options()("everest-dir", po::value<std::string>()->default_value(defaults::RUNTIME_DIR),
                       fmt::format("Set the everest runtime directory (default: {})", defaults::RUNTIME_DIR).c_str());
    desc.add_options()(
        "config", po::value<std::string>(),
        fmt::format("Set the main configuration file path (default: {})", DEFAULT_CONFIG_FILE_PATH).c_str());
    desc.add_options()(
        "modules-dir", po::value<std::string>(),
        fmt::format("Set the directory for module definitions (default: {})", DEFAULT_MODULE_DIR).c_str());
    desc.add_options()(
        "logging-config", po::value<std::string>(),
        fmt::format("Set the logging configuration file path (default: {})", DEFAULT_LOGGING_CONFIG_FILE_PATH).c_str());

    po::positional_options_description p;

    // FIXME (aw): add these
    // desc.add_options()("dump", po::value<std::string>(),
    //                    "Dump validated and augmented main config and all used module manifests into dir");
    // desc.add_options()("dumpmanifests", po::value<std::string>(),
    //                    "Dump manifests of all modules into dir (even modules not used in config) and exit");
    // desc.add_options()("standalone,s", po::value<std::vector<std::string>>()->multitoken(),
    //                    "Module ID(s) to not automatically start child processes for (those must be started manually
    //                    to " "make the framework start!).");
    // desc.add_options()("ignore", po::value<std::vector<std::string>>()->multitoken(),
    //                    "Module ID(s) to ignore: Do not automatically start child processes and do not require that "
    //                    "they are started.");
    // desc.add_options()("dontvalidateschema", "Don't validate json schema on every message");

    po::variables_map vm;

    try {
        po::store(po::command_line_parser(argc, argv)
                      .options(desc)
                      .positional(p)
                      .style(po::command_line_style::default_style & ~po::command_line_style::allow_guessing)
                      .run(),
                  vm);
        po::notify(vm);
    } catch (const po::error& e) {
        std::stringstream usage;
        desc.print(usage);
        throw RuntimeEnvironmentException(fmt::format("{}\nUsage error: {}", usage.str(), e.what()));
    }

    if (vm.count("help")) {
        std::stringstream usage;
        desc.print(usage);
        throw RuntimeEnvironmentException(fmt::format("{}", usage.str()), true);
    }

    rs.runtime_path = resolve_path_if_exists(vm["everest-dir"].as<std::string>());
    if (!fs::is_directory(rs.runtime_path)) {
        throw RuntimeEnvironmentException(
            fmt::format("EVerest runtime directory '{}' does not exist", rs.runtime_path.string()));
    }

    auto config_path =
        resolve_path_if_exists(get_option_or_default(vm, "config", rs.runtime_path / defaults::CONFIG_FILE));
    if (!fs::is_regular_file(config_path)) {
        throw RuntimeEnvironmentException(fmt::format("EVerest config file '{}' not found", config_path.string()));
    }
    fs::load_string_file(config_path, rs.config);

    rs.modules_path =
        resolve_path_if_exists(get_option_or_default(vm, "modules-dir", rs.runtime_path / defaults::MODULE_SUB_DIR));
    if (!fs::is_directory(rs.modules_path)) {
        throw RuntimeEnvironmentException(
            fmt::format("EVerest modules directory '{}' does not exist", rs.modules_path.string()));
    }

    rs.logging_config_path = resolve_path_if_exists(
        get_option_or_default(vm, "logging-config", rs.runtime_path / defaults::LOGGING_CONFIG_FILE));
    if (!fs::is_regular_file(rs.logging_config_path)) {
        throw RuntimeEnvironmentException(
            fmt::format("EVerest logging config file '{}' not found", rs.logging_config_path.string()));
    }

    return rs;
}
