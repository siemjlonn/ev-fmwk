#include <fmt/core.h>
#include <thread>

#include <everest/module_peer.hpp>
#include <everest/peer.hpp>
#include <everest/utils/mqtt_io_mqttc.hpp>
#include <everest/utils/schema.hpp>

#include "../testing_helpers.hpp"

void simple_manager() {
    everest::mqtt::MQTTC_IO mqtt_io(everest::mqtt::get_default_server_address());
    everest::Peer manager("manager", mqtt_io);

    auto io_sync_thread = std::thread([&mqtt_io]() {
        while (mqtt_io.sync(50))
            ;
    });

    manager.implement_command("", "say_hello", [](everest::Arguments args) -> everest::Value {
        if (args.at("module_id") == "mod_client") {
            return {
                {"connections",
                 {{"server",
                   {
                       {
                           {"module_id", "mod_server"},
                           {"implementation_id", ""},
                       },
                   }}}},
                {"config", 23},
            };
        } else {
            return {{"config", 10}};
        }
        return nullptr;
    });

    auto num_clients = 0;

    std::promise<void> ready_promise;

    manager.implement_command("", "init_done", [&num_clients, &ready_promise](everest::Arguments args) {
        if (2 == ++num_clients) {
            // publish_ready();
            ready_promise.set_value();
        }
        return nullptr;
    });

    ready_promise.get_future().wait();
    manager.publish_variable("", "ready", nullptr);

    io_sync_thread.join();
}

void server() {
    auto ping_server_intf =
        everest::schema::parse_interface(read_resource_file("ping_pong/interfaces/ping_server.json"));

    everest::ModulePeer server(
        everest::ModuleBuilder("mod_server", "srv_hash").add_implementation("", {"ping_server", {}}, ping_server_intf));

    auto io_sync_exited = server.spawn_io_sync_thread();

    const auto& module_config = server.say_hello();
    size_t status_until_ping_count = module_config.get_config_sets();
    auto server_received_pings = 0;

    server.implement_command("", "ping", [&server_received_pings](const everest::Arguments& args) {
        server_received_pings++;
        return fmt::format("Hello '{}'", args.at("source"));
    });

    server.init_done();

    while (server_received_pings < status_until_ping_count) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        server.publish_variable("", "status", server_received_pings);
    }

    io_sync_exited.wait();
}

void client() {
    auto ping_server_intf =
        everest::schema::parse_interface(read_resource_file("ping_pong/interfaces/ping_server.json"));

    everest::ModulePeer client(
        everest::ModuleBuilder("mod_client", "cli_hash").add_requirement("server", {"ping_server", 1, 1}, ping_server_intf));

    auto io_sync_exited = client.spawn_io_sync_thread();

    auto config = client.say_hello();
    fmt::print("Client got config: {}\n", config.get_config_sets().dump(2));

    auto server_fulfillment = config.get_fulfillments("server").front();

    client.subscribe_variable(server_fulfillment, "status", [](const everest::Value& status) {
        fmt::print("Client got a status var: {}\n", status.dump(2));
    });

    client.init_done();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        auto retval = client.call_command(server_fulfillment, "ping", {{"source", "client one"}});
        fmt::print("Server response: {}\n", retval.dump(2));
    }

    io_sync_exited.wait();
}

int main(int argc, char* argv[]) {
    auto manager_thread = std::thread(simple_manager);
    auto server_thread = std::thread(server);

    client();

    manager_thread.join();
    server_thread.join();

    return 0;
}
