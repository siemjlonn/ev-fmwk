#include <everest/module_peer.hpp>

#include <future>

#include <fmt/core.h>

#include <everest/constants.hpp>
#include <everest/peer.hpp>
#include <everest/utils/mqtt_io_base.hpp>
#include <everest/utils/mqtt_io_mqttc.hpp>

namespace everest {

ModulePeer::ModulePeer(const Module& module, std::unique_ptr<mqtt::IOBase> mqtt_io) :
    module(module), mqtt_io(std::move(mqtt_io)), peer(std::make_unique<Peer>(module.id, *this->mqtt_io)) {
}

ModulePeer::ModulePeer(const std::string& module_id) : ModulePeer(Module{module_id, {}, {}}) {
}

ModulePeer::ModulePeer(const Module& module) :
    ModulePeer(module, std::make_unique<mqtt::MQTTC_IO>(mqtt::get_default_server_address())) {
}

ModulePeer::~ModulePeer() {
    if (this->sync_thread_running) {
        this->sync_thread_running = false;
        if (this->sync_thread.joinable()) {
            this->sync_thread.join();
        }
    }
}

const Module& ModulePeer::get_module() const {
    return this->module;
}

Value ModulePeer::call_command(const Fulfillment& fulfillment, const std::string& command_name,
                               const Arguments& args) const {
    return this->peer->call_command(fulfillment.module_id, fulfillment.implementation_id, command_name, args);
}

void ModulePeer::publish_variable(const std::string& implementation_id, const std::string& variable_name,
                                  const Value& value) const {
    this->peer->publish_variable(implementation_id, variable_name, value);
}

void ModulePeer::implement_command(const std::string& implementation_id, const std::string& command_name,
                                   const CommandHandler& handler) const {
    this->peer->implement_command(implementation_id, command_name, handler);
}

UnsubscriptionCallback ModulePeer::subscribe_variable(const Fulfillment& fulfillment, const std::string& variable_name,
                                                      const SubscriptionHandler& handler) const {
    return this->peer->subscribe_variable(fulfillment.module_id, fulfillment.implementation_id, variable_name, handler);
}

UnsubscriptionCallback ModulePeer::mqtt_subscribe(const std::string& topic,
                                                           const MQTTSubscriptionHandler& handler) {
    return this->peer->mqtt_subscribe(topic, handler);
}

void ModulePeer::mqtt_publish(const std::string& topic, const std::string& data) {
    this->peer->mqtt_publish(topic, data);
}

void ModulePeer::bypass(const nlohmann::json& bypass_data) {
    if (this->bypassed) {
        throw std::runtime_error("This module peer can only be bypassed once");
    }

    this->bypassed = true;

    // FIXME (aw): the config should be validated
    this->config.setup(this->module, bypass_data);
}

const ModuleConfig& ModulePeer::say_hello() {
    if (this->state != State::Constructed) {
        throw std::runtime_error("Module is not allowed to say hello in its current state");
    }

    if (!this->bypassed) {
        if (this->sync_thread_running == false) {
            throw std::runtime_error("This function would block forever because the IO sync thread is not running");
        }

        auto module_config =
            this->peer->call_command("manager", "", "say_hello", everest::Arguments{{"module_id", this->module.id}});
        this->config.setup(this->module, module_config);
    }

    this->state = State::Booted;

    return this->config;
}

void ModulePeer::init_done() {
    // FIXME (aw): check if everything is implemented

    if (this->state != State::Booted) {
        throw std::runtime_error("Module is not allow to finish initialization in its current state");
    }

    if (this->bypassed) {
        this->state = State::Initialized;
        return;
    }

    // subscribe to managers ready
    auto ready_promise = std::make_shared<std::promise<void>>();
    auto ready_future = std::move(ready_promise->get_future());
    auto unsubscribe_from_ready = this->peer->subscribe_variable("manager", "", "ready", [ready_promise](Value var) {
        try {
            ready_promise->set_value();
            return true;
        } catch (const std::future_error& e) {
            if (e.code() == std::future_errc::promise_already_satisfied) {
                // this could happen if the ready got delivered multiple times ...
                return false;
            }
            throw;
        }
    });

    this->peer->call_command("manager", "", "init_done", {{"module_id", this->peer->peer_id}});

    ready_future.wait();
    unsubscribe_from_ready();
    this->state = State::Initialized;
}

std::future<void> ModulePeer::spawn_io_sync_thread() {

    if (this->sync_thread_running) {
        throw std::runtime_error("IO sync thread has been already spawned");
    }
    this->sync_thread_running = true;

    auto task = std::packaged_task<void(void)>([this]() {
        while (this->sync_thread_running) {
            // FIXME (aw): what about this polling timeout?
            this->mqtt_io->sync(50);
        }
    });

    auto future = task.get_future();
    this->sync_thread = std::thread(std::move(task));

    return future;
}

} // namespace everest
