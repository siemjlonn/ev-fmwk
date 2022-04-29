// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_RUNTIME_HPP
#define EVEREST_RUNTIME_HPP

#include <sys/prctl.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

#include <everest/logging/logging.hpp>

namespace everest {

namespace po = boost::program_options;
namespace fs = boost::filesystem;

// FIXME (aw): should be everest wide or defined in liblog
const int DUMP_INDENT = 4;

// FIXME (aw): we should also define all other config keys and default
//             values here as string literals

const auto TERMINAL_STYLE_ERROR = fmt::emphasis::bold | fg(fmt::terminal_color::red);
const auto TERMINAL_STYLE_OK = fmt::emphasis::bold | fg(fmt::terminal_color::green);

struct RuntimeSettings {
    fs::path main_dir;
    fs::path main_binary;
    fs::path configs_dir;
    fs::path schemas_dir;
    fs::path modules_dir;
    fs::path interfaces_dir;
    fs::path logging_config;
    fs::path config_file;
    bool validate_schema;

    explicit RuntimeSettings(const po::variables_map& vm);
};

} // namespace everest

#endif // EVEREST_RUNTIME_HPP
