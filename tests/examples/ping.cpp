#include <fmt/core.h>
#include <thread>

#include <everest/peer.hpp>
#include <everest/utils/mqtt_io_mqttc.hpp>

int main(int argc, char* argv[]) {

    // setup manager
    everest::mqtt::MQTTC_IO mqtt_io_master(everest::mqtt::get_default_server_address());
    everest::Peer manager("manager", mqtt_io_master);

    manager.implement_command("", "register", [&manager](const nlohmann::json& args) {
        fmt::print("Received arguments for register: {}\n", args.dump(2));
        // manager.publish_variable("impl1", "var_name", {{"zz", "top"}});
        return nlohmann::json{{"echo", args}};
    });

    // setup module
    everest::mqtt::MQTTC_IO mqtt_io_module(everest::mqtt::get_default_server_address());
    everest::Peer mod("module", mqtt_io_module);

    auto module_thread = std::thread([&mod]() {
        // give the manager some time to subscribe its topics
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        const auto result = mod.call_command("manager", "", "register", "Module says hi!");
        fmt::print("Module got back an answer: {}\n", result.dump(2));
    });

    while (true) {
        mqtt_io_master.sync(50);
        mqtt_io_module.sync(50);
    }

    module_thread.join();

    return 0;
}
