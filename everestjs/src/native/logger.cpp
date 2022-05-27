// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include "logger.hpp"

#include <everest/logging/logging.hpp>

inline static std::string extract_logstring(const Napi::CallbackInfo& info) {
    // TODO (aw): check input
    return info[0].ToString().Utf8Value();
}

void init_logger(Napi::Env env, Napi::Object exports) {

    exports.DefineProperty(Napi::PropertyDescriptor::Value(
        "EverestLogger",
        Napi::Function::New(
            env,
            [](const Napi::CallbackInfo& info) {
                const auto logging_config_file = info[0].As<Napi::String>();
                if (info.Length() == 2) {
                    const auto process_name = info[1].As<Napi::String>();
                    everest::logging::init(logging_config_file, process_name);
                } else {
                    everest::logging::init(logging_config_file);
                }

                auto env = info.Env();

                Napi::Object log = Napi::Object::New(env);
                log.DefineProperty(Napi::PropertyDescriptor::Value(
                    "debug",
                    Napi::Function::New(
                        env, [](const Napi::CallbackInfo& info) { EVLOG(debug) << extract_logstring(info); }),
                    napi_enumerable));
                log.DefineProperty(Napi::PropertyDescriptor::Value(
                    "info",
                    Napi::Function::New(env,
                                        [](const Napi::CallbackInfo& info) { EVLOG(info) << extract_logstring(info); }),
                    napi_enumerable));
                log.DefineProperty(Napi::PropertyDescriptor::Value(
                    "warning",
                    Napi::Function::New(
                        env, [](const Napi::CallbackInfo& info) { EVLOG(warning) << extract_logstring(info); }),
                    napi_enumerable));
                log.DefineProperty(Napi::PropertyDescriptor::Value(
                    "error",
                    Napi::Function::New(
                        env, [](const Napi::CallbackInfo& info) { EVLOG(error) << extract_logstring(info); }),
                    napi_enumerable));
                log.DefineProperty(Napi::PropertyDescriptor::Value(
                    "critical",
                    Napi::Function::New(
                        env, [](const Napi::CallbackInfo& info) { EVLOG(critical) << extract_logstring(info); }),
                    napi_enumerable));

                return log;
            },
            "everest_init_logging"),
        napi_enumerable));
}
