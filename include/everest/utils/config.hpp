// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_UTILS_CONFIG_HPP
#define EVEREST_UTILS_CONFIG_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "schema.hpp"

namespace everest {

struct ConfigValidationExecption : public std::runtime_error {
    ConfigValidationExecption(const std::string& error) : std::runtime_error(error){};
};

class ModuleManifestLoaderBase {
public:
    schema::ModuleManifest get_module_manifest(const std::string& module_type) const;
    virtual ~ModuleManifestLoaderBase() = default;

protected:
    virtual std::string get_raw_manifest(const std::string& module_type) const = 0;
};

class Config {
public:
    struct ModuleDescription {
        std::string type;
        schema::ModuleManifest manifest;
        schema::ModuleConfiguration config;
    };
    using ModuleDescriptions = std::unordered_map<std::string, ModuleDescription>;
    Config(const ModuleManifestLoaderBase& loader, const std::string& config);
    Config(const ModuleManifestLoaderBase& loader, const nlohmann::json& config);

    const auto& get_modules() {
        return this->modules;
    }

private:
    void validate_fulfillments(const std::string& module_id, const std::string& required_interface,
                               const nlohmann::json& fulfillments) const;
    ModuleDescriptions modules;
    nlohmann::json config;
};

} // namespace everest

#endif // EVEREST_UTILS_CONFIG_HPP
