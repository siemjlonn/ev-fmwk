// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVERESTJS_CONVERSION_HPP
#define EVERESTJS_CONVERSION_HPP

#include <napi.h>

#include <everest/types.hpp>

everest::Value napi_value_to_ev_value(const Napi::Value& value);

Napi::Value ev_value_to_napi_value(Napi::Env env, const everest::Value& value);

inline Napi::Value message_to_value(Napi::Env env, const std::string& value) {
    return Napi::Value::From(env, value);
}

inline Napi::Value message_to_value(Napi::Env env, const everest::Value& value) {
    return ev_value_to_napi_value(env, value);
}

inline everest::Fulfillment napi_object_to_fulfillment(const Napi::Object& value) {
    return {
        value.Get("implementation_id").As<Napi::String>(),
        value.Get("module_id").As<Napi::String>(),
    };
}

#endif // EVERESTJS_CONVERSION_HPP
