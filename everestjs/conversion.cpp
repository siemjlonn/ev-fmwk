// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest

#include "conversion.hpp"

#include <cmath>
#include <everest/logging/exceptions.hpp>
#include <everest/logging/logging.hpp>

static const char* const napi_valuetype_strings[] = {
    "undefined", //
    "null",      //
    "boolean",   //
    "number",    //
    "string",    //
    "symbol",    //
    "object",    //
    "function",  //
    "external",  //
    "bigint",    //
};

using json = nlohmann::json;

// FIXME (aw): how to treat the special objects like promises etc ...?
everest::Value napi_value_to_ev_value(const Napi::Value& value) {
    BOOST_LOG_FUNCTION();

    const auto type = value.Type();
    if (type == napi_null || type == napi_undefined) {
        return nullptr;
    } else if (type == napi_boolean) {
        return static_cast<bool>(value.As<Napi::Boolean>());
    } else if (type == napi_number) {
        const auto number = value.As<Napi::Number>();
        double integral_part;
        if (0 == std::modf(number, &integral_part)) {
            return static_cast<int64_t>(number);
        } else {
            return static_cast<double>(number);
        }
    } else if (type == napi_string) {
        return static_cast<std::string>(value.As<Napi::String>());
    } else if (value.IsArray()) {
        auto array_json = json::array();
        const auto array_napi = value.As<Napi::Array>();
        for (size_t i = 0; i < array_napi.Length(); ++i) {
            array_json[i] = napi_value_to_ev_value(array_napi[i]);
        }
        return array_json;
    } else if (type == napi_object) {
        auto object_json = json::object();
        const auto object_napi = value.As<Napi::Object>();
        const auto keys = object_napi.GetPropertyNames();
        for (size_t i = 0; i < keys.Length(); ++i) {
            object_json[keys[i].As<Napi::String>()] = napi_value_to_ev_value(object_napi[keys[i]]);
        }
        return object_json;
    }
    EVTHROW(EVEXCEPTION(everest::EverestApiError, "Javascript type can not be converted to everest::json: ",
                        napi_valuetype_strings[value.Type()]));
}

Napi::Value ev_value_to_napi_value(Napi::Env env, const json& value) {
    BOOST_LOG_FUNCTION();

    if (value.is_null()) {
        return env.Null();
    } else if (value.is_string()) {
        return Napi::String::New(env, static_cast<std::string>(value));
    } else if (value.is_number_integer()) {
        return Napi::Number::New(env, static_cast<int64_t>(value));
    } else if (value.is_number_float()) {
        return Napi::Number::New(env, static_cast<double>(value));
    } else if (value.is_boolean()) {
        return Napi::Boolean::New(env, static_cast<bool>(value));
    } else if (value.is_array()) {
        Napi::Array array_napi = Napi::Array::New(env);
        for (const auto& item : value.items()) {
            array_napi.Set(item.key(), ev_value_to_napi_value(env, item.value()));
        }
        return array_napi;
    } else if (value.is_object()) {
        Napi::Object object_napi = Napi::Object::New(env);
        for (const auto& item : value.items()) {
            object_napi.Set(item.key(), ev_value_to_napi_value(env, item.value()));
        }
        return object_napi;
    }
    EVTHROW(EVEXCEPTION(everest::EverestApiError, "Javascript type can not be converted to Napi::Value: ", value));
}
