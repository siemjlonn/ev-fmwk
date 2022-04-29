// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_GENERATED_MODULE_HPP
#define EVEREST_GENERATED_MODULE_HPP

#include <string>

#include "generated_module_fwd.hpp"
#include "module_peer.hpp"
#include "utils/schema.hpp"
#include <everest/interface.hpp>
#include <everest/logging/logging.hpp>

namespace everest::generated {

inline void publish(ImplementationContext& ctx, const schema::VariableType& variable_type,
                    const std::string& variable_name, const Value& value) {

    const auto validation_result = value_matches_variable_type(value, variable_type);
    if (!validation_result) {
        throw std::runtime_error("Published variable does not against its schema");
    }

    ctx.module_peer.publish_variable(ctx.implementation_id, variable_name, value);
}

inline Value call(RequirementContext& ctx, const schema::CommandType& command_type, const std::string& command_name,
                  const Arguments& args) {

    auto validation_result = arguments_match_command_type(args, command_type);
    if (!validation_result) {
        throw std::runtime_error("Supplied arguments, that do not validate against its schema");
    }

    auto result = ctx.module_peer.call_command(ctx.fulfillment, command_name, args);

    validation_result = return_value_matches_command_type(result, command_type);
    if (!validation_result) {
        throw std::runtime_error("Got result which does not validate against its schema");
    }

    return result;
}

inline void implement(const ImplementationContext& ctx, const std::string& command_name,
                      const CommandHandler& handler) {
    const auto& command_type =
        ctx.module_peer.get_module().get_command_type_for_implementation(ctx.implementation_id, command_name);

    ctx.module_peer.implement_command(
        ctx.implementation_id, command_name, [handler, command_type](const Arguments& args) {
            // FIXME (aw): need to check here, that the command_type is copied!

            auto validation_result = arguments_match_command_type(args, command_type);
            if (!validation_result) {
                throw std::runtime_error("Command got called with arguments that do not validate against its schema");
            }

            auto result = handler(args);

            validation_result = return_value_matches_command_type(result, command_type);
            if (!validation_result) {
                throw std::runtime_error(
                    "Implemented command returned a result which does not validate against its schema");
            }

            return result;
        });
}

inline UnsubscriptionCallback subscribe(RequirementContext& ctx, const schema::VariableType& variable_type,
                                        const std::string& variable_name, const SubscriptionHandler& handler) {

    // NOTE (aw): passing the handler by reference might be enough here
    return ctx.module_peer.subscribe_variable(
        ctx.fulfillment, variable_name, [handler, &variable_type](const Value& value) {
            auto validation_result = value_matches_variable_type(value, variable_type);
            if (!validation_result) {
                throw std::runtime_error("Got published value that does not validate against its schema");
            }

            handler(value);
        });
}

template <typename BootRequirementType, typename RunRequirementType> struct RequirementHelper {
    std::vector<BootRequirementType> boot{};
    std::vector<RunRequirementType> run{};
    RequirementHelper(ModulePeer& module_peer, const std::vector<Fulfillment>& fulfillments,
                      const std::string& req_id) {
        boot.reserve(fulfillments.size());
        run.reserve(fulfillments.size());
        for (const auto& fulfillment : fulfillments) {
            boot.emplace_back(RequirementContext{module_peer, fulfillment, req_id});
            run.emplace_back(RequirementContext{module_peer, fulfillment, req_id});
        }
    }
};

inline nullptr_t safe_cast_to_null(const nlohmann::json& value_json) {
    if (value_json.is_null()) {
        return value_json;
    }

    throw std::runtime_error("JSON value does not represent a null value");
}

inline bool safe_cast_to_boolean(const nlohmann::json& value_json) {
    if (value_json.is_boolean()) {
        return value_json;
    }

    throw std::runtime_error("JSON value does not represent a boolean value");
}

inline int safe_cast_to_integer(const nlohmann::json& value_json) {
    if (value_json.is_number_integer()) {
        return value_json;
    }

    throw std::runtime_error("JSON value does not represent an integer number");
}

inline double safe_cast_to_number(const nlohmann::json& value_json) {
    // NOTE (aw): we also allow integers here, because otherwise one
    // needs to explicitely add the '.' at the end of every floating
    // point number that can be represented as an integer
    if (value_json.is_number()) {
        return value_json;
    }

    throw std::runtime_error("JSON value does not represent a floating point number");
}

inline std::string safe_cast_to_string(const nlohmann::json& value_json) {
    if (value_json.is_string()) {
        return value_json;
    }

    throw std::runtime_error("JSON value does not represent a string");
}

inline const nlohmann::json& safe_cast_to_object(const nlohmann::json& value_json) {
    if (value_json.is_object()) {
        return value_json;
    }

    throw std::runtime_error("JSON value does not represent an object");
}

inline const nlohmann::json& safe_cast_to_array(const nlohmann::json& value_json) {
    if (value_json.is_array()) {
        return value_json;
    }

    throw std::runtime_error("JSON value does not represent an object");
}

inline const nlohmann::json& safe_cast_to_variant(const nlohmann::json& value_json) {
    // FIXME (aw): this would not really check if the variant contains only the requested types!
    return value_json;
}

} // namespace everest::generated

#endif // EVEREST_GENERATED_MODULE_HPP
