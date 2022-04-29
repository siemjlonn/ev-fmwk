// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_INTERFACE_HPP
#define EVEREST_INTERFACE_HPP

#include <map>
#include <nlohmann/json.hpp>
#include <vector>

#include <everest/types.hpp>
#include <everest/utils/schema.hpp>

namespace everest {

bool arguments_match_command_type(const Arguments& arguments, const schema::CommandType& command_type);

schema::ValidationResult value_matches_variable_type(const Value& value, const schema::VariableType& variable_type);

schema::ValidationResult return_value_matches_command_type(const Value& return_value, const schema::CommandType& cmd_type);

} // namespace everest

#endif // EVEREST_INTERFACE_HPP
