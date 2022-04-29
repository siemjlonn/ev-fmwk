// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest

#include <everest/utils/mqtt_io_base.hpp>

namespace everest::mqtt {

IOBase::IOBase(const std::function<void(const RawMessage&)>& message_handler) : handler(message_handler) {
}

IOBase::IOBase() :
    IOBase([](const RawMessage& msg) {
        // FIXME (aw): we could log missing messages here ...
    }) {
}

void IOBase::set_handler(const std::function<void(const RawMessage&)>& message_handler) {
    this->handler = message_handler;
}

} // namespace everest::mqtt
