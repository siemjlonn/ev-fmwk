// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_EVEREST_HPP
#define EVEREST_EVEREST_HPP

#include <functional>
#include <string>

#include <nlohmann/json.hpp>

namespace everest {

using Value = nlohmann::json;
using Arguments = nlohmann::json;

using CommandHandler = std::function<Value(const Arguments&)>;
using PublishFunction = std::function<void(const Value&)>;
using CallFunction = std::function<Value(const Arguments&)>;
using MQTTSubscriptionHandler = std::function<void(const std::string&)>;
using SubscriptionHandler = std::function<void(const Value&)>;

using UnsubscriptionCallback = std::function<void(void)>;

// type related helper routines
std::string format_value(Value value);
std::string format_arguments(Arguments arguments);

} // namespace everest

#endif // EVEREST_EVEREST_HPP
