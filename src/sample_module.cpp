// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <thread>

#include <fmt/core.h>

#include <everest/peer.hpp>
#include <everest/utils/mqtt_io_mqttc.hpp>

int main(int argc, char* argv[]) {
    std::string module_id{argv[0]};

    fmt::print("Module with id '{}' started", module_id);

    everest::mqtt::MQTTC_IO mqtt_io(everest::mqtt::get_default_server_address());
    everest::Peer mod(module_id, mqtt_io);

    mod.subscribe_variable("manager", "", "ready", [](const everest::Value& value) {
        fmt::print("Got a ready\n");
    });

    auto mod_thread = std::thread([&mod]() {
        auto result = mod.call_command("manager", "", "hello_from", everest::Arguments{{ "module_id", mod.peer_id }});
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        result = mod.call_command("manager", "", "init_done", everest::Arguments{{ "module_id", mod.peer_id }});
    });

    while (true) {
        mqtt_io.sync(50);
    }

    return 0;
}
