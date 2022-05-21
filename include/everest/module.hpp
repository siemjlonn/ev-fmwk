// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_MODULE_HPP
#define EVEREST_MODULE_HPP

#include <map>
#include <string>

#include <everest/types.hpp>
#include <everest/utils/schema.hpp>

namespace everest {

class Module {
public:
    using InterfaceMap = std::unordered_map<std::string, schema::Interface>;

    Module(const std::string& id, schema::ModuleManifest manifest, InterfaceMap interfaces);

    const schema::CommandType& get_command_type_for_implementation(const std::string& implementation_id,
                                                                   const std::string& command_name) const;
    const schema::VariableType& get_variable_type_for_implementation(const std::string& implementation_id,
                                                                     const std::string& variable_name) const;
    const schema::CommandType& get_command_type_for_requirement(const std::string& requirement_id,
                                                                const std::string& command_name) const;
    const schema::VariableType& get_variable_type_for_requirement(const std::string& requirement_id,
                                                                  const std::string& variable_name) const;

    bool has_implementable_commands() const;

    const std::string id;
    const schema::ModuleManifest manifest;
    const InterfaceMap interfaces;

private:
    const schema::Interface& get_implementation_interface(const std::string implementation_id) const;
    const schema::Interface& get_requirement_interface(const std::string requirement_id) const;
};

class ModuleBuilder {
public:
    explicit ModuleBuilder(const std::string& module_id, const std::string& hash);
    ModuleBuilder& add_implementation(const std::string& id, const schema::Implementation& def,
                                      const schema::Interface& interface);
    ModuleBuilder& add_requirement(const std::string& id, const schema::Requirement& def,
                                   const schema::Interface& interface);
    operator Module() const;

private:
    bool insert_interface(const std::string& interface_name, const schema::Interface& interface);

    std::string module_id;
    Module::InterfaceMap interfaces{};
    schema::ModuleManifest manifest{};
};

class InterfaceMapBuilder {
public:
    operator Module::InterfaceMap() const;
    InterfaceMapBuilder& add(const std::string& name, const std::string& interface);
private:
    Module::InterfaceMap interfaces;
};

class ModuleConfig {
public:
    using ConnectionMap = std::unordered_map<std::string, std::vector<Fulfillment>>;
    void setup(const Module& module, const nlohmann::json& module_setup);

    const nlohmann::json& get_config_sets() const;
    const std::vector<Fulfillment>& get_fulfillments(const std::string& requirement_id) const;

private:
    bool setup_done {false};
    nlohmann::json config_sets{nlohmann::json::object()};
    ConnectionMap connections{};
};

} // namespace everest

#endif // EVEREST_MODULE_HPP
