#include <everest/utils/schema.hpp>

#include <memory>

#include <fmt/core.h>
#include <nlohmann/json-schema.hpp>

#include <everest/utils/helpers.hpp>
#include <everest/utils/os.hpp>

// include our inline manifests
#include <generated/schema/config.h>
#include <generated/schema/interface.h>
#include <generated/schema/manifest.h>

// FIXME (aw): check format_checker interface
static void format_checker(const std::string& format, const std::string& value) {
    if (format == "uri") {
        if (value.find("://") == std::string::npos) {
            // FIXME (aw): this doesn't really validate uri's ...
            throw std::invalid_argument(value + " is not a valid uri according to RFC ....");
        }
    } else {
        nlohmann::json_schema::default_string_format_check(format, value);
    }
}

// FIXME (aw): this should be converted to constexpr so we get static initialization
static everest::schema::SchemaValidator create_schema_validator_or_die(const char* schema, size_t len,
                                                                       const char* schema_ref) {
    try {
        return everest::schema::SchemaValidator(schema, len);
    } catch (const std::exception& e) {
        // FIXME (aw): exception handling
        everest::os::unrecoverable_error(
            fmt::format("Could not create SchemaValidator (schema reference: '{}').\nReason: {}\nTerminating.",
                        schema_ref, e.what()));
    }
}

namespace everest::schema {

using nlohmann::json;
using json_validator = nlohmann::json_schema::json_validator;

class ValidationErrorHandler : public nlohmann::json_schema::basic_error_handler {
    void error(const nlohmann::json_pointer<nlohmann::basic_json<>>& pointer, const json& instance,
               const std::string& message) override {
        nlohmann::json_schema::basic_error_handler::error(pointer, instance, message);
        this->result.error = message;
        this->result.instance = instance;
        this->result.pointer = pointer;
    }
    ValidationResult& result;

public:
    ValidationErrorHandler(everest::schema::ValidationResult& result) : result(result) {
    }
};

SchemaValidator::SchemaValidator(const char* schema, size_t len) try :
    SchemaValidator(json::parse(schema, schema + len, nullptr, true, true)) {
} catch (const json::parse_error& e) {
    // FIXME (aw): exception handling
    throw std::runtime_error(fmt::format("Supplied schema cannot be parsed as JSON\n{}", e.what()));
}

SchemaValidator::SchemaValidator(const json& schema) :
    schema(schema), validator(std::make_shared<json_validator>(nullptr, format_checker)) {
    try {
        validator->set_root_schema(this->schema);
    } catch (const std::exception& e) {
        // FIXME (aw): exception handling
        throw std::runtime_error(fmt::format("Supplied schema is not a valid JSON schema\n{}", e.what()));
    }
}

ValidationResult SchemaValidator::validate(const json& input) const {
    everest::schema::ValidationResult result;
    ValidationErrorHandler error_handler{result};
    auto patch = this->validator->validate(input, error_handler);
    if (!error_handler) {
        result.patch = patch;
    }

    return result;
}

ValidationResult::operator bool() const {
    return this->error.empty();
}

static SchemaValidator interface_validator =
    create_schema_validator_or_die(interface_json, interface_json_len, "interface, inlined");
static SchemaValidator module_validator =
    create_schema_validator_or_die(manifest_json, manifest_json_len, "module, inlined");
static SchemaValidator config_validator = create_schema_validator_or_die(config_json, config_json_len, "config");

ValidationResult validate_interface(const json& input) noexcept {
    return interface_validator.validate(input);
}

ValidationResult validate_module(const json& input) noexcept {
    return module_validator.validate(input);
}

ValidationResult validate_config(const json& input) noexcept {
    return config_validator.validate(input);
}

Schema get_module_schema() {
    return {
        manifest_json,
        manifest_json_len,
        manifest_json_md5,
    };
}

Schema get_config_schema() {
    return {
        config_json,
        config_json_len,
        config_json_md5,
    };
}

Schema get_interface_schema() {
    return {
        interface_json,
        interface_json_len,
        interface_json_md5,
    };
}

static auto parse_config_set(const nlohmann::json& config_set_json) {
    std::unordered_map<std::string, SchemaValidator> config_set;

    for (const auto& config_item : config_set_json.items()) {
        const auto pair = config_set.emplace(config_item.key(), SchemaValidator(config_item.value()));

        // if there is a default value, lets check if it validates against its own schema!
        const auto default_it = config_item.value().find("default");
        if (default_it != config_item.value().end()) {
            const auto& validator = pair.first->second;
            if (!validator.validate(*default_it)) {
                // FIXME (aw): exception handling
                throw std::runtime_error(fmt::format(
                    "Default value for config key '{}' does not validate against its own schema", config_item.key()));
            }
        }
    }

    return config_set;
}

static auto parse_capabilities(const nlohmann::json& capabilities_json) {
    std::vector<std::string> capabilities;

    for (const auto& cap : capabilities_json.items()) {
        capabilities.emplace_back(cap.value());
    };

    return capabilities;
}

static auto parse_metadata(const nlohmann::json& module_info_json) {
    
    std::vector<std::string> authors;
    authors.reserve(module_info_json.at("authors").size());

    for (const auto& author : module_info_json.at("authors").items()) {
        authors.emplace_back(author.value());
    }

    return Metadata {
        std::move(authors),
        module_info_json.at("license"),
    };
}

static auto parse_implementations(const nlohmann::json& implementations_json) {
    std::unordered_map<std::string, Implementation> impls;

    for (const auto& impl : implementations_json.items()) {
        Implementation impl_desc;

        auto config_it = impl.value().find("config");
        if (config_it != impl.value().end()) {
            impl_desc.config_schemas = parse_config_set(*config_it);
        }

        impl_desc.interface = impl.value().at("interface");

        impls.emplace(impl.key(), std::move(impl_desc));
    }

    return impls;
}

static auto parse_requirements(const nlohmann::json& requirements_json) {
    std::unordered_map<std::string, Requirement> reqs;

    for (const auto& req : requirements_json.items()) {
        Requirement req_desc;

        req_desc.interface = req.value().at("interface");
        req_desc.min_connections = req.value().value("min_connections", 1);
        req_desc.max_connections = req.value().value("max_connections", 1);

        reqs.emplace(req.key(), std::move(req_desc));
    }

    return reqs;
}

ModuleManifest parse_module(const nlohmann::json& module_json, const std::string& hash) {
    auto validation_result = schema::validate_module(module_json);

    if (!validation_result) {
        // FIXME (aw): exception handling
        throw std::runtime_error(
            fmt::format("{}\njson_ptr: {}\n", validation_result.error, validation_result.pointer.to_string()));
    }

    // ok module is valid, fill the struct

    schema::ModuleManifest manifest;

    manifest.hash = hash;

    // NOTE (aw): this iterator check pattern could be templatized ...
    const auto caps_it = module_json.find("capabilities");
    if (caps_it != module_json.end()) {
        manifest.capabilities = parse_capabilities(*caps_it);
    }

    const auto config_it = module_json.find("config");
    if (config_it != module_json.end()) {
        manifest.config_schemas = parse_config_set(*config_it);
    }

    const auto impls_it = module_json.find("implements");
    if (impls_it != module_json.end()) {
        manifest.implementations = parse_implementations(*impls_it);
    }

    const auto reqs_it = module_json.find("requires");
    if (reqs_it != module_json.end()) {
        manifest.requirements = parse_requirements(*reqs_it);
    }

    manifest.metadata = parse_metadata(module_json.at("metadata"));

    return manifest;
}

ModuleManifest parse_module(const std::string& module_text) {
    auto module_json = nlohmann::json::parse(module_text, nullptr, false);

    if (module_json.is_discarded()) {
        throw std::runtime_error("Module definition cannot be parsed as JSON");
    }

    const auto module_hash = utils::md5_hash(module_text.data(), module_text.length());

    return parse_module(module_json, module_hash);
}

static auto parse_config_set(const nlohmann::json& config, const ConfigSetSchemas& schemas) {
    nlohmann::json config_set;

    // check all needed config items from the manifest
    for (const auto& schema_it : schemas) {
        const auto& validator = schema_it.second;
        const auto default_value = validator.schema.value("default", nlohmann::json(nullptr));

        const auto& schema_item_name = schema_it.first;

        const auto config_item_it = config.find(schema_item_name);
        if (config_item_it != config.end()) {
            // check if it validates
            const auto& cm_item_value = config_item_it.value();
            if (!validator.validate(cm_item_value)) {
                // FIXME (aw): exception handling
                throw std::runtime_error(
                    fmt::format("Supplied value for config key '{}' does not validate against the config keys schema",
                                schema_item_name));
            }

            config_set.emplace(schema_item_name, cm_item_value);
        } else {
            if (default_value.is_null()) {
                // FIXME (aw): exception handling
                throw std::runtime_error(
                    fmt::format("Config key '{}' in config set is not set and has no default key", schema_item_name));
            }

            config_set.emplace(schema_item_name, default_value);
        }
    }

    // cross check if no settings were done, that do not exist in the manifest
    for (const auto& config_item : config.items()) {
        if (schemas.count(config_item.key()) == 0) {
            // FIXME (aw): exception handling
            throw std::runtime_error(
                fmt::format("Config key '{}' has been set in the configuration, but does not exist in the manifest",
                            config_item.key()));
        }
    }

    return config_set;
}
static Type parse_type(nlohmann::json type_json) {
    return type_json;
}

// FIXME (aw): where can we use const& for nlohmann::json?
static ArgumentTypes parse_arguments(nlohmann::json args_json) {
    ArgumentTypes args;

    for (auto& arg : args_json.items()) {
        args.emplace(arg.key(), parse_type(arg.value()));
    }

    return args;
}

static CommandType parse_command(const nlohmann::json& cmd_json) {
    return {
        parse_arguments(cmd_json.value("arguments", nlohmann::json::object())),
        parse_type(cmd_json.value("result", nlohmann::json::object())),
    };
}

static std::map<std::string, CommandType> parse_commands(const nlohmann::json& cmds_json) {
    std::map<std::string, CommandType> cmds;

    for (auto& cmd : cmds_json.items()) {
        cmds.emplace(cmd.key(), parse_command(cmd.value()));
    }

    return cmds;
}

static VariableType parse_variable(const nlohmann::json& var_json) {
    return {parse_type(var_json)};
}

static std::map<std::string, VariableType> parse_variables(const nlohmann::json& vars_json) {
    std::map<std::string, VariableType> vars;

    for (auto& var : vars_json.items()) {
        vars.emplace(var.key(), parse_variable(var.value()));
    }

    return vars;
}

schema::Interface parse_interface(const nlohmann::json& intf_json, const std::string& hash) {
    auto valid = schema::validate_interface(intf_json);
    if (!valid) {
        // FIXME (aw): exception handling
        throw std::runtime_error("Could not validate interface");
    }

    return {
        parse_commands(intf_json.value("cmds", nlohmann::json::object())),
        parse_variables(intf_json.value("vars", nlohmann::json::object())),
        hash,
    };
}

Interface parse_interface(const std::string& interface_text) {
    auto interface_json = nlohmann::json::parse(interface_text, nullptr, false);

    if (interface_json.is_discarded()) {
        throw std::runtime_error("Interface definition cannot be parsed as JSON");
    }

    const auto interface_hash = utils::md5_hash(interface_text.data(), interface_text.length());

    return parse_interface(interface_json, interface_hash);
}

static auto parse_implementation_configuration(const nlohmann::json& config_implementation,
                                               const std::unordered_map<std::string, Implementation>& implementations) {
    nlohmann::json config_impl_set;

    // check all available implementations
    for (const auto& impl_it : implementations) {
        const auto& impl_name = impl_it.first;
        const auto& impl_config = config_implementation.value(impl_name, nlohmann::json::object());

        try {
            config_impl_set.emplace(impl_name, parse_config_set(impl_config, impl_it.second.config_schemas));
        } catch (const std::exception& e) {
            // FIXME (aw): exception handling
            throw std::runtime_error(
                fmt::format("Failed to parse the config set for implementation id '{}'\n{}", impl_name, e.what()));
        }
    }

    // cross check if implementations have been configured, that do not exist
    for (const auto& impl_item : config_implementation.items()) {
        if (implementations.count(impl_item.key()) == 0) {
            // FIXME (aw): exception handling
            throw std::runtime_error(fmt::format(
                "Configuration found for an implementation named '{}', that does not exist in the module manifest",
                impl_item.key()));
        }
    }

    return config_impl_set;
}

static void check_connections(const nlohmann::json& connections_json,
                              const std::unordered_map<std::string, Requirement>& requirements) {
    // check if all needed requirements are specified
    for (const auto& req_it : requirements) {
        const auto& req_id = req_it.first;

        const auto& req_conn_it = connections_json.find(req_id);

        if (req_conn_it == connections_json.end()) {
            // requirement does not exist in config

            if (req_it.second.min_connections > 0) {
                // FIXME (aw): exception handling
                throw std::runtime_error(fmt::format(
                    "Requirement with id '{}' needs at least one connection to a fulfilling implementation", req_id));
            }
        } else {
            // requirement does exist and should be of type array
            if (req_it.second.max_connections < req_conn_it.value().size()) {
                throw std::runtime_error(
                    // FIXME (aw): exception handling
                    fmt::format("Requirement with id '{}' can take at maximum '{}' connections to fulfilling "
                                "implementations, but '{}' have been defined in the config",
                                req_id, req_it.second.max_connections, req_conn_it.value().size()));
            }
        }
    }

    // cross check if requirements have been specified that do not exist
    for (const auto& req_item : connections_json.items()) {
        if (requirements.count(req_item.key()) == 0) {
            // FIXME (aw): exception handling
            throw std::runtime_error(
                fmt::format("Connection found for a requirement named '{}', that does not exist in the module manifest",
                            req_item.key()));
        }
    }
}

ModuleConfiguration parse_module_configuration(const nlohmann::json& module_config_json,
                                               const ModuleManifest& manifest) {
    auto config_sets = nlohmann::json::object();

    // check config_*
    const auto& config_cm = module_config_json.value("config_module", nlohmann::json::object());
    try {
        config_sets.emplace("module", parse_config_set(config_cm, manifest.config_schemas));
    } catch (const std::exception& e) {
        // FIXME (aw): exception handling
        throw std::runtime_error(fmt::format("Failed to parse the config set for the module\n{}", e.what()));
    }

    const auto& config_ci = module_config_json.value("config_implementation", nlohmann::json::object());
    config_sets.emplace("implementations", parse_implementation_configuration(config_ci, manifest.implementations));

    const auto& connection_config = module_config_json.value("connections", nlohmann::json::object());
    check_connections(connection_config, manifest.requirements);

    return {
        {"config", std::move(config_sets)},
        {"connections", connection_config},
    };
}

} // namespace everest::schema
