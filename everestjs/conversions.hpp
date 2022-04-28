// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVERESTJS_CONVERSIONS_HPP
#define EVERESTJS_CONVERSIONS_HPP

#include <napi.h>

#include <everest/everest.hpp>
#include <everest/types.hpp>
#include <everest/utils/conversions.hpp>

namespace everest {

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

everest::json convertToJson(const Napi::Value& value);
Napi::Value convertToNapiValue(const Napi::Env& env, const everest::json& value);

} // namespace everest

#endif // EVERESTJS_CONVERSIONS_HPP
