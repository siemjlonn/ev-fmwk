// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_UTILS_OS_HPP
#define EVEREST_UTILS_OS_HPP

#include <string>

namespace everest::os {

[[noreturn]] void unrecoverable_error(const std::string& message);

std::string get_environment_variable(const std::string& key);

} // namespace everest::os

#endif // EVEREST_UTILS_OS_HPP
