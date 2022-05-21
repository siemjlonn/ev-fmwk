// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest

#include <stdio.h>

#include <boost/filesystem.hpp>

#include <fmt/format.h>

#include <everest/logging/logging.hpp>
#include <everest/utils/config.hpp>

#include "manager.hpp"
#include "runtime_env.hpp"

class ModuleManifestLoader : public everest::ModuleManifestLoaderBase {
public:
    ModuleManifestLoader(const boost::filesystem::path& module_path) :
        ModuleManifestLoaderBase(), module_path(module_path){};

private:
    std::string get_raw_manifest(const std::string& module_type) const override final {
        const auto manifest_path = resolve_path_if_exists(module_path / module_type / "manifest.json");
        if (!boost::filesystem::is_regular_file(manifest_path)) {
            // FIXME (aw): exception handling
            throw std::runtime_error(
                fmt::format("Manifest for module '{}' does not exist at '{}'", module_type, manifest_path.string()));
        }

        std::string raw_manifest;
        boost::filesystem::load_string_file(manifest_path, raw_manifest);

        return raw_manifest;
    }

    boost::filesystem::path module_path;
};

void boot(const RuntimeEnvironment& rs) {
    EVLOG(info) << "EVerest manager says hi!";
    try {
        Manager manager{{ModuleManifestLoader(rs.modules_path), rs.config}};

        // FIXME (aw): should manager get the full runtime config?
        manager.run(rs);

    } catch (const everest::ConfigValidationExecption& e) {
        EVLOG(error) << fmt::format("Could not start manager because of invalid config:\n{}\n", e.what());
    }
    EVLOG(info) << "EVerest manager says goodbye!";
}

int main(int argc, char* argv[]) {

    try {
        const auto rs = handle_command_line_args(argc, argv);
        everest::logging::init(rs.logging_config_path.string());

        if (rs.only_validate) {
            try {
                everest::Config(ModuleManifestLoader(rs.modules_path), rs.config);
                fmt::print(stdout, "Config file valid\n");
                return EXIT_SUCCESS;
            } catch (const everest::ConfigValidationExecption& e) {
                fmt::print(stdout, "Could not validate config file.\n{}\n", e.what());
                return EXIT_FAILURE;
            }
        }

        boot(rs);

    } catch (const RuntimeEnvironmentException& e) {
        if (e.show_usage) {
            fmt::print(stdout, "{}\n", e.what());
            return EXIT_SUCCESS;
        }

        fmt::print(stderr, "{}\n", e.what());
        return EXIT_FAILURE;

    } catch (const std::exception& e) {
        fmt::print(stderr, "{}\nExiting ...\n", e.what());
        return EXIT_FAILURE;
    }
}
