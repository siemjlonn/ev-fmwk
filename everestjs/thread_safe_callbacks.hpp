// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVERESTJS_THREAD_SAFE_CALLBACKS_HPP
#define EVERESTJS_THREAD_SAFE_CALLBACKS_HPP

#include <future>
#include <mutex>
#include <string>
#include <queue>

#include <napi.h>

#include <everest/types.hpp>

template <typename MessageType> struct AsyncSubscriptionTraits;

template <> struct AsyncSubscriptionTraits<everest::Value> { static constexpr auto resource_name = "var_subscribe"; };
template <> struct AsyncSubscriptionTraits<std::string> { static constexpr auto resource_name = "mqtt_subscribe"; };

template <typename MessageType> struct AsyncSubscription {
    static void js_trampoline(Napi::Env env, Napi::Function callback, nullptr_t*, AsyncSubscription* this_) {
        std::unique_lock lock(this_->queue_mutex);
        auto value = message_to_value(env, this_->messages.front());
        this_->messages.pop();
        lock.unlock();
        callback.Call({value});
    };

    using TSFN = Napi::TypedThreadSafeFunction<nullptr_t, AsyncSubscription, AsyncSubscription::js_trampoline>;

    AsyncSubscription(const std::string& topic, Napi::Env env, Napi::Function handler) : topic(topic) {
        this->tsfn = TSFN::New(env, handler, AsyncSubscriptionTraits<MessageType>::resource_name, 0, 1);
    }

    void dispatch(const MessageType& message) {
        {
            const std::lock_guard lock(this->queue_mutex);
            this->messages.push(message);
        }
        this->tsfn.BlockingCall(this);
    }

    std::mutex queue_mutex{};
    std::queue<MessageType> messages{};
    std::string topic;
    TSFN tsfn;
};

struct AsyncCmdImplementation {
    static void js_trampoline(Napi::Env env, Napi::Function callback, nullptr_t*, AsyncCmdImplementation* this_);

    using TSFN =
        Napi::TypedThreadSafeFunction<nullptr_t, AsyncCmdImplementation, AsyncCmdImplementation::js_trampoline>;

    AsyncCmdImplementation(const std::string& topic, Napi::Env env, Napi::Function handler);

    everest::Value dispatch(const everest::Arguments& args);

    const everest::Arguments* arguments;
    std::promise<everest::Value>* result;

    std::string topic;
    TSFN tsfn;
};

#endif // EVERESTJS_THREAD_SAFE_CALLBACKS_HPP
