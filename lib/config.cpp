// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <fmt/format.h>

#include <everest/utils/config.hpp>
#include <everest/utils/helpers.hpp>

namespace everest {

schema::ModuleManifest ModuleManifestLoaderBase::get_module_manifest(const std::string& module_type) const {
    const auto manifest_raw = this->get_raw_manifest(module_type);

    const auto manifest_json = nlohmann::json::parse(manifest_raw, nullptr, false);

    if (manifest_json.is_discarded()) {
        // FIXME (aw): exception handling
        throw std::runtime_error("Module manifest is not valid json.");
    }

    const auto manifest_hash = utils::md5_hash(manifest_raw.data(), manifest_raw.size());

    return schema::parse_module(manifest_json, manifest_hash);
}

inline void Config::validate_fulfillments(const std::string& module_id, const std::string& required_interface,
                                          const nlohmann::json& fulfillments) const {
    for (const auto& fulfillment : fulfillments.items()) {
        // check for self reference
        const std::string& fulfilling_module_id = fulfillment.value().at("module_id");
        const std::string& fulfilling_implementation_id = fulfillment.value().at("implementation_id");
        if (fulfilling_module_id == module_id) {
            // FIXME (aw): exception handling
            throw std::runtime_error("A module is not allowed to fulfill its own requirements");
        }

        auto fulfilling_module_it = modules.find(fulfilling_module_id);
        if (fulfilling_module_it == this->modules.end()) {
            // FIXME (aw): exception handling
            throw std::runtime_error(
                fmt::format("The module with id '{}' is not listed in the config and cannot fulfill the requirement",
                            fulfilling_module_id));
        }

        // fulfilling module exists, check for implementation
        const auto& fulfilling_mod_manifest = fulfilling_module_it->second.manifest;
        const auto fulfilling_impl_it = fulfilling_mod_manifest.implementations.find(fulfilling_implementation_id);
        if (fulfilling_impl_it == fulfilling_mod_manifest.implementations.end()) {
            // FIXME (aw): exception handling
            throw std::runtime_error(fmt::format(
                "The module with id '{}' has no implementation with id '{}' that could fulfill the requirement",
                fulfilling_module_id, fulfilling_implementation_id));
        }

        // implementation exists, check for interface match
        if (fulfilling_impl_it->second.interface != required_interface) {
            // FIXME (aw): exception handling
            throw std::runtime_error(fmt::format("The interface type '{}' of implementation with id '{}' of module "
                                                 "with id '{}' does not match the requirements interface '{}'",
                                                 fulfilling_impl_it->second.interface, fulfilling_implementation_id,
                                                 fulfilling_module_id, required_interface));
        }
    }
}

Config::Config(const ModuleManifestLoaderBase& loader, const nlohmann::json& config_json) : config(config_json) {
    if (config.is_discarded()) {
        // FIXME (aw): exception handling
        throw ConfigValidationExecption("Config is not valid json.");
    }

    const auto validation_result = schema::validate_config(config);
    if (!validation_result) {
        // FIXME (aw): exception handling
        throw ConfigValidationExecption(fmt::format("Config did not validate against config schema\n{}\njson_ptr: {}",
                                                    validation_result.error, validation_result.pointer.to_string()));
    }

    for (const auto& mod_json : config.items()) {
        const auto module_id = mod_json.key();
        const std::string module_type = mod_json.value().at("module");

        try {
            auto manifest = loader.get_module_manifest(module_type);
            auto config = schema::parse_module_configuration(mod_json.value(), manifest);
            this->modules.emplace(module_id, ModuleDescription{
                                                 module_type,
                                                 std::move(manifest),
                                                 std::move(config),
                                             });
        } catch (const std::exception& e) {
            // FIXME (aw): handle exceptions
            throw ConfigValidationExecption(fmt::format(
                "Could not parse configuration for module id '{}' of type '{}'\n{}", module_id, module_type, e.what()));
        }
    }

    // check connections - check also for self references!
    for (const auto& mod : this->modules) {
        const auto& module_id = mod.first;
        const auto& mod_conf = mod.second.config;
        for (const auto& req_con : mod_conf.at("connections").items()) {
            const auto& required_interface = mod.second.manifest.requirements.at(req_con.key()).interface;
            try {
                this->validate_fulfillments(module_id, required_interface, req_con.value());
            } catch (const std::exception& e) {
                throw ConfigValidationExecption(
                    fmt::format("Requirement with id '{}' of module with id '{}' has an invalid fulfillment.\n{}",
                                req_con.key(), module_id, e.what()));
            }
        }
    }
}

Config::Config(const ModuleManifestLoaderBase& loader, const std::string& config_raw) :
    Config(loader, nlohmann::json::parse(config_raw, nullptr, false)) {
}

} // namespace everest
