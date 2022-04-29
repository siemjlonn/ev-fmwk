// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest

#include <everest/runtime.hpp>

namespace everest {

RuntimeSettings::RuntimeSettings(const po::variables_map& vm) : main_dir(vm["main_dir"].as<std::string>()) {
    if (vm.count("schemas_dir")) {
        schemas_dir = vm["schemas_dir"].as<std::string>();
    } else {
        schemas_dir = main_dir / "schemas";
    }

    if (vm.count("modules_dir")) {
        modules_dir = vm["modules_dir"].as<std::string>();
    } else {
        modules_dir = main_dir / "modules";
    }

    if (vm.count("interfaces_dir")) {
        interfaces_dir = vm["interfaces_dir"].as<std::string>();
    } else {
        interfaces_dir = main_dir / "interfaces";
    }

    if (vm.count("log_conf")) {
        logging_config = vm["log_conf"].as<std::string>();
    } else {
        logging_config = main_dir / "conf/logging.ini";
    }

    if (vm.count("conf")) {
        config_file = vm["conf"].as<std::string>();
    } else {
        config_file = main_dir / "conf/config.json";
    }

    validate_schema = (vm.count("dontvalidateschema") != 0);

    // make all paths canonical
    std::reference_wrapper<fs::path> list[] = {
        main_dir, configs_dir, schemas_dir, modules_dir, interfaces_dir, logging_config, config_file,
    };

    for (auto ref_wrapped_item : list) {
        auto& item = ref_wrapped_item.get();
        item = fs::canonical(item);
    }
}

} // namespace everest
