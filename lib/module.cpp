#include <everest/module.hpp>

#include <fmt/core.h>

#include <everest/utils/helpers.hpp>
#include <everest/logging/logging.hpp>

namespace everest {

// static std::map<std::string, Interface>
// check_interfaces(const std::map<std::string, InterfaceDefinition>& interface_map) {
//     std::map<std::string, Interface> defs;
//     for (auto& intf : interface_map) {
//         defs[intf.first] = parse_interface(intf.second);
//     }

//     return defs;
// }

ModuleBuilder::ModuleBuilder(const std::string& module_id, const std::string& hash) : module_id(module_id) {
    this->manifest.hash = hash;
}

bool ModuleBuilder::insert_interface(const std::string& interface_name, const schema::Interface& interface) {
    const auto intf_it = this->interfaces.find(interface_name);
    if (intf_it == this->interfaces.end()) {
        // interface not yet seen, add it
        this->interfaces.emplace(interface_name, interface);
    } else {
        // already there, check if the hashes match
        if (intf_it->second.hash != interface.hash) {
            return false;
        }
    }

    return true;
}

ModuleBuilder& ModuleBuilder::add_implementation(const std::string& id, const schema::Implementation& def,
                                                 const schema::Interface& interface) {
    if (this->manifest.implementations.count(id) != 0) {
        throw std::runtime_error(fmt::format("Implementation with id '{}' already exists", id));
    }

    if (!this->insert_interface(def.interface, interface)) {
        throw std::runtime_error(fmt::format(
            "Interface '{}', supplied by implementation '{}' already exists with a different hash", def.interface, id));
    }

    this->manifest.implementations.emplace(id, def);

    return *this;
}

ModuleBuilder& ModuleBuilder::add_requirement(const std::string& id, const schema::Requirement& def,
                                              const schema::Interface& interface) {
    if (this->manifest.requirements.count(id) != 0) {
        throw std::runtime_error(fmt::format("Requirement with id '{}' already exists", id));
    }

    if (!this->insert_interface(def.interface, interface)) {

        throw std::runtime_error(fmt::format(
            "Interface '{}', supplied by requirement '{}' already exists with a different hash", def.interface, id));
    }

    this->manifest.requirements.emplace(id, def);

    return *this;
}

ModuleBuilder::operator Module() const {
    return {this->module_id, this->manifest, this->interfaces};
}

InterfaceMapBuilder::operator Module::InterfaceMap() const {
    return this->interfaces;
}

InterfaceMapBuilder& InterfaceMapBuilder::add(const std::string& name, const std::string& interface) {
    const auto intf_it = this->interfaces.find(name);
    if (intf_it == this->interfaces.end()) {
        // interface not yet seen, add it
        this->interfaces.emplace(name, schema::parse_interface(interface));
    } else {
        // already there, check if the hashes match
        if (intf_it->second.hash != utils::md5_hash(interface.data(), interface.length())) {
            throw std::runtime_error(
                fmt::format("Interface '{}' has been added already, but with a different hash", name));
        }
    }
    return *this;
}

Module::Module(const std::string& id, schema::ModuleManifest manifest, InterfaceMap interfaces) :
    id(id), manifest(std::move(manifest)), interfaces(std::move(interfaces)) {
}

static const schema::CommandType& get_command_type(const schema::Interface& interface, const std::string& name) {
    try {
        return interface.commands.at(name);
    } catch (const std::out_of_range& e) {
        // FIXME (aw): exception
        throw std::runtime_error("Intf id does not have cmd");
    }
}

static const schema::VariableType& get_variable_type(const schema::Interface& interface, const std::string& name) {
    try {
        return interface.variables.at(name);
    } catch (const std::out_of_range& e) {
        // FIXME (aw): exception
        throw std::runtime_error("Intf id does not have cmd");
    }
}

const schema::Interface& Module::get_implementation_interface(const std::string implementation_id) const {
    const auto impl_it = this->manifest.implementations.find(implementation_id);
    if (impl_it == this->manifest.implementations.end()) {
        throw std::runtime_error("Impl id does not exist");
    }

    const auto intf_it = this->interfaces.find(impl_it->second.interface);
    if (intf_it == this->interfaces.end()) {
        throw std::runtime_error("Intf does not exist, fatal");
    }

    return intf_it->second;
}

const schema::Interface& Module::get_requirement_interface(const std::string requirement_id) const {
    const auto req_it = this->manifest.requirements.find(requirement_id);
    if (req_it == this->manifest.requirements.end()) {
        throw std::runtime_error("Req id does not exist");
    }

    const auto intf_it = this->interfaces.find(req_it->second.interface);
    if (intf_it == this->interfaces.end()) {
        throw std::runtime_error("Intf does not exist, fatal");
    }

    return intf_it->second;
}

const schema::CommandType& Module::get_command_type_for_implementation(const std::string& implementation_id,
                                                                       const std::string& command_name) const {

    const auto& intf = get_implementation_interface(implementation_id);
    return get_command_type(intf, command_name);
}

const schema::VariableType& Module::get_variable_type_for_implementation(const std::string& implementation_id,
                                                                         const std::string& variable_name) const {
    const auto& intf = get_implementation_interface(implementation_id);
    return get_variable_type(intf, variable_name);
}

const schema::CommandType& Module::get_command_type_for_requirement(const std::string& requirement_id,
                                                                    const std::string& command_name) const {
    const auto& intf = get_requirement_interface(requirement_id);
    return get_command_type(intf, command_name);
}

const schema::VariableType& Module::get_variable_type_for_requirement(const std::string& requirement_id,
                                                                      const std::string& variable_name) const {
    const auto& intf = get_requirement_interface(requirement_id);
    return get_variable_type(intf, variable_name);
}

bool Module::has_implementable_commands() const {
    for (const auto& impl_it : this->manifest.implementations) {
        const auto& interface_name = impl_it.second.interface;
        if (this->interfaces.at(interface_name).commands.size() > 0) {
            return true;
        }
    }

    return false;
}

const nlohmann::json& ModuleConfig::get_config_sets() const {
    if (!this->setup_done) {
        throw std::runtime_error("The module setup has not been done yet");
    }
    return this->config_sets;
}

const std::vector<Fulfillment>& ModuleConfig::get_fulfillments(const std::string& requirement_id) const {
    if (!this->setup_done) {
        throw std::runtime_error("The module setup has not been done yet");
    }

    auto fulfillments_it = this->connections.find(requirement_id);

    if (fulfillments_it == this->connections.end()) {
        throw std::runtime_error(fmt::format("No fulfillment has been set up for requirement '{}'", requirement_id));
    }

    return fulfillments_it->second;
}

void ModuleConfig::setup(const Module& module, const nlohmann::json& module_setup) {
    if (this->setup_done) {
        throw std::runtime_error("The module setup has been done already");
    }

    const auto& connections_json = module_setup.value("connections", nlohmann::json::object());

    // check requirements
    for (const auto& req_it : module.manifest.requirements) {
        const auto& req_id = req_it.first;
        const auto& requirement = req_it.second;

        const auto fulfillments_json = connections_json.value(req_id, nlohmann::json::array());
        std::vector<Fulfillment> fulfillments;

        const auto fulfillments_count = fulfillments_json.size();

        if (fulfillments_count < requirement.min_connections) {
            throw std::runtime_error(
                fmt::format("Cannot setup module, because there are less fulfillments than needed ({} < {}) for "
                            "the requirement id '{}'",
                            fulfillments_count, requirement.min_connections, req_id));
        }

        if (fulfillments_count > requirement.max_connections) {
            throw std::runtime_error(
                fmt::format("Cannot setup module, because there are more fulfillments than needed ({} > {}) for "
                            "the requirement id '{}'",
                            fulfillments_count, requirement.max_connections, req_id));
        }

        fulfillments.reserve(fulfillments_count);

        for (const auto& fulfillment_json : fulfillments_json.items()) {
            fulfillments.emplace_back(Fulfillment{
                fulfillment_json.value().at("module_id"),
                fulfillment_json.value().at("implementation_id"),
            });
        }

        this->connections.emplace(req_id, std::move(fulfillments));
    }

    // cross check
    for (const auto& fulfillments_json : connections_json.items()) {
        if (module.manifest.requirements.count(fulfillments_json.key()) == 0) {
            throw std::runtime_error(
                fmt::format("Found fulfillemnts for unknown requirement id '{}'", fulfillments_json.key()));
        }
    }

    // FIXME (do config check!)
    this->config_sets = module_setup.value("config", nlohmann::json::object());

    this->setup_done = true;
}

} // namespace everest
