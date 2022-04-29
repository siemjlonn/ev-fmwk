// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_UTILS_SCHEMA_HPP
#define EVEREST_UTILS_SCHEMA_HPP

#include <memory>

#include <nlohmann/json.hpp>

namespace nlohmann::json_schema {

// forward declaration
class json_validator;

} // namespace nlohmann::json_schema

namespace everest::schema {

struct Schema {
    const char* text;
    const size_t byte_size;
    const char* md5;
};

struct ValidationResult {
    operator bool() const;
    std::string error{};
    nlohmann::json_pointer<nlohmann::basic_json<>> pointer{};
    nlohmann::json instance{};
    nlohmann::json patch{};
};

class SchemaValidator {
public:
    SchemaValidator(const char* schema, size_t len);
    SchemaValidator(const nlohmann::json& schema);
    ValidationResult validate(const nlohmann::json& input) const;

    const nlohmann::json schema;

private:
    std::shared_ptr<nlohmann::json_schema::json_validator> validator;
};

using ConfigSetSchemas = std::unordered_map<std::string, SchemaValidator>;

using Type = schema::SchemaValidator;
using ArgumentType = Type;
using ArgumentTypes = std::map<std::string, ArgumentType>;
using VariableType = Type;
using ReturnType = Type;

struct CommandType {
    ArgumentTypes arguments;
    ReturnType return_type;
};

struct Interface {
    std::map<std::string, CommandType> commands;
    std::map<std::string, VariableType> variables;
    std::string hash;
};

struct Metadata {
    std::vector<std::string> authors;
    std::string license;
};

struct Implementation {
    std::string interface;
    ConfigSetSchemas config_schemas;
};

struct Requirement {
    std::string interface;
    size_t min_connections;
    size_t max_connections;
    bool is_vector() const {
        return (min_connections != 1) || (max_connections != 1);
    }
};

struct ModuleManifest {
    std::vector<std::string> capabilities;
    ConfigSetSchemas config_schemas;
    std::unordered_map<std::string, Implementation> implementations;
    std::unordered_map<std::string, Requirement> requirements;
    Metadata metadata;
    std::string hash;
};

using ModuleConfiguration = nlohmann::json;

ModuleManifest parse_module(const nlohmann::json& module_json, const std::string& hash);
ModuleManifest parse_module(const std::string& module_text);


Interface parse_interface(const nlohmann::json& interface_json, const std::string& hash);
Interface parse_interface(const std::string& interface_text);

ModuleConfiguration parse_module_configuration(const nlohmann::json& module_config_json,
                                               const ModuleManifest& manifest);

Schema get_module_schema();
Schema get_config_schema();
Schema get_interface_schema();

ValidationResult validate_interface(const nlohmann::json& input) noexcept;
ValidationResult validate_module(const nlohmann::json& input) noexcept;
ValidationResult validate_config(const nlohmann::json& input) noexcept;

// parse subscription message
// parse command message
// parse result message

} // namespace everest::schema

#endif // EVEREST_UTILS_SCHEMA_HPP
