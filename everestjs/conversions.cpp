// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
#include "conversions.hpp"

#include <everest/exceptions.hpp>
#include <everest/logging.hpp>

namespace everest_js {

everest::json convertToJson(const Napi::Value& value) {
    BOOST_LOG_FUNCTION();

    if (value.IsNull() || value.IsUndefined()) {
        return everest::json(nullptr);
    } else if (value.IsString()) {
        return everest::json(std::string(value.As<Napi::String>()));
    } else if (value.IsNumber()) {
        int64_t intNumber = value.As<Napi::Number>();
        double floatNumber = value.As<Napi::Number>();
        if (floatNumber == intNumber)
            return everest::json(intNumber);
        return everest::json(floatNumber);
    } else if (value.IsBoolean()) {
        return everest::json(bool(value.As<Napi::Boolean>()));
    } else if (value.IsArray()) {
        auto j = everest::json::array();
        Napi::Array array = value.As<Napi::Array>();
        for (uint64_t i = 0; i < array.Length(); i++) {
            Napi::Value entry = Napi::Value(array[i]);
            j[i] = convertToJson(entry);
        }
        return j;
    } else if (value.IsObject() && value.Type() == napi_object) {
        auto j = everest::json({});
        Napi::Object obj = value.As<Napi::Object>();
        Napi::Array keys = obj.GetPropertyNames();
        for (uint64_t i = 0; i < keys.Length(); i++) {
            Napi::Value key = keys[i];
            if (key.IsString()) {
                std::string k = key.As<Napi::String>();
                Napi::Value v = Napi::Value(obj[k]);
                j[k] = convertToJson(v);
            } else {
                EVTHROW(EVEXCEPTION(everest::EverestApiError,
                                    "Javascript type of object key can not be converted to Everest::json: ",
                                    napi_valuetype_strings[key.Type()]));
            }
        }
        return j;
    }
    EVTHROW(EVEXCEPTION(everest::EverestApiError, "Javascript type can not be converted to Everest::json: ",
                        napi_valuetype_strings[value.Type()]));
}

Napi::Value convertToNapiValue(const Napi::Env& env, const json& value) {
    BOOST_LOG_FUNCTION();

    if (value.is_null()) {
        return env.Null();
    } else if (value.is_string()) {
        return Napi::String::New(env, std::string(value));
    } else if (value.is_number_integer()) {
        return Napi::Number::New(env, int64_t(value));
    } else if (value.is_number_float()) {
        return Napi::Number::New(env, double(value));
    } else if (value.is_boolean()) {
        return Napi::Boolean::New(env, bool(value));
    } else if (value.is_array()) {
        Napi::Array v = Napi::Array::New(env);
        for (uint64_t i = 0; i < value.size(); i++) {
            v.Set(i, convertToNapiValue(env, value[i]));
        }
        return v;
    } else if (value.is_object()) {
        Napi::Object v = Napi::Object::New(env);
        for (auto& el : value.items()) {
            v.Set(el.key(), convertToNapiValue(env, el.value()));
        }
        return v;
    }
    EVTHROW(EVEXCEPTION(everest::EverestApiError, "Javascript type can not be converted to Napi::Value: ", value));
}

} // namespace everest_js
