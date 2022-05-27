// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include "thread_safe_callbacks.hpp"

#include <napi.h>

#include "conversion.hpp"

void AsyncCmdImplementation::js_trampoline(Napi::Env env, Napi::Function callback, nullptr_t*,
                                           AsyncCmdImplementation* this_) {
    const auto resolve = Napi::Function::New(env, [&result = this_->result](const Napi::CallbackInfo& info) {
        result->set_value(napi_value_to_ev_value(info[0]));
        return info.Env().Null();
    });
    const auto reject = Napi::Function::New(env, [&result = this_->result](const Napi::CallbackInfo& info) {
        result->set_exception(std::make_exception_ptr(std::runtime_error("Command handler reject result")));
        return info.Env().Null();
    });
    const auto value = callback.Call({ev_value_to_napi_value(env, *this_->arguments), resolve, reject});
}

AsyncCmdImplementation::AsyncCmdImplementation(const std::string& topic, Napi::Env env, Napi::Function handler) :
    topic(topic) {
    this->tsfn = TSFN::New(env, handler, "cmd_implementation", 0, 1);
}

everest::Value AsyncCmdImplementation::dispatch(const everest::Arguments& args) {
    std::promise<everest::Value> result;
    this->arguments = &args;
    this->result = &result;

    this->tsfn.BlockingCall(this);

    return result.get_future().get();
}
