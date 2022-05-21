// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <forward_list>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>

#include <napi.h>

#include <boost/filesystem.hpp>

#include <everest/logging/exceptions.hpp>
#include <everest/logging/logging.hpp>
#include <everest/module_peer.hpp>
#include <everest/utils/helpers.hpp>

#include "conversion.hpp"
#include "logger.hpp"
#include "thread_safe_callbacks.hpp"
#include "utils.hpp"

static std::string read_file(const std::string& dir, const std::string& name) {
    namespace fs = boost::filesystem;
    std::string data;

    auto fs_path = boost::filesystem::path(dir) / name;
    // FIXME (aw): check if exists

    fs_path = fs::exists(fs_path) ? fs::canonical(fs_path) : fs_path;

    if (!fs::is_regular_file(fs_path)) {
        throw std::runtime_error("Could not open " + fs_path.string());
    }

    boost::filesystem::load_string_file(fs_path, data);

    return data;
}

class EverestModule : public Napi::ObjectWrap<EverestModule> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    EverestModule(const Napi::CallbackInfo& info);

private:
    using AsyncMQTTSubscription = AsyncSubscription<std::string>;
    using AsyncVarSubscription = AsyncSubscription<everest::Value>;

    Napi::Value say_hello(const Napi::CallbackInfo& info);
    Napi::Value init_done(const Napi::CallbackInfo& info);

    Napi::Value call_command(const Napi::CallbackInfo& info);
    Napi::Value publish_variable(const Napi::CallbackInfo& info);
    Napi::Value implement_command(const Napi::CallbackInfo& info);
    Napi::Value subscribe_variable(const Napi::CallbackInfo& info);
    Napi::Value mqtt_publish(const Napi::CallbackInfo& info);
    Napi::Value mqtt_subscribe(const Napi::CallbackInfo& info);

    Napi::Value get_metadata(const Napi::CallbackInfo& info);

    std::unique_ptr<everest::ModulePeer> peer;
    std::list<AsyncMQTTSubscription> mqtt_subscriptions;
    std::list<AsyncVarSubscription> var_subscriptions;
    std::list<AsyncCmdImplementation> cmd_implementations;
};

Napi::Object EverestModule::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(
        env, "EverestModule",
        {
            InstanceMethod<&EverestModule::say_hello>(
                "say_hello", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
            InstanceMethod<&EverestModule::init_done>(
                "init_done", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
            InstanceMethod<&EverestModule::call_command>(
                "call_command", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
            InstanceMethod<&EverestModule::publish_variable>(
                "publish_variable", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
            InstanceMethod<&EverestModule::implement_command>(
                "implement_command", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
            InstanceMethod<&EverestModule::subscribe_variable>(
                "subscribe_variable", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
            InstanceMethod<&EverestModule::mqtt_publish>(
                "mqtt_publish", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
            InstanceMethod<&EverestModule::mqtt_subscribe>(
                "mqtt_subscribe", static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
            InstanceAccessor<&EverestModule::get_metadata>("metadata"),
        });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();

    *constructor = Napi::Persistent(func);
    exports.Set("EverestModule", func);

    env.SetInstanceData<Napi::FunctionReference>(constructor);

    return exports;
}

EverestModule::EverestModule(const Napi::CallbackInfo& info) : Napi::ObjectWrap<EverestModule>(info) {
    Napi::Env env = info.Env();

    const Napi::Object& settings = info[0].ToObject();
    const auto& module_id = settings.Get("module_id").ToString().Utf8Value();
    const auto& module_dir = settings.Get("module_dir").ToString().Utf8Value();
    const auto& interfaces_dir = settings.Get("interfaces_dir").ToString().Utf8Value();

    const auto manifest_text = read_file(module_dir, "manifest.json");
    const auto manifest = everest::schema::parse_module(manifest_text);

    auto interface_map_builder = everest::InterfaceMapBuilder();
    for (const auto impl_it : manifest.implementations) {
        const auto& interface_type = impl_it.second.interface;
        interface_map_builder.add(interface_type, read_file(interfaces_dir, interface_type + ".json"));
    }

    for (const auto req_it : manifest.requirements) {
        const auto& interface_type = req_it.second.interface;
        interface_map_builder.add(interface_type, read_file(interfaces_dir, interface_type + ".json"));
    }

    this->peer =
        std::make_unique<everest::ModulePeer>(everest::Module{module_id, std::move(manifest), interface_map_builder});

    // FIXME (aw): how to wait / die ?
    auto io_sync_finished = this->peer->spawn_io_sync_thread();
}

Napi::Value EverestModule::get_metadata(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    auto module_info = Napi::Object::New(env);

    const auto& manifest = this->peer->get_module().manifest;

    module_info.DefineProperty(
        Napi::PropertyDescriptor::Value("license", Napi::String::New(env, manifest.metadata.license), napi_enumerable));

    auto authors = Napi::Array::New(env, manifest.metadata.authors.size());
    for (size_t i = 0; i < manifest.metadata.authors.size(); ++i) {
        authors[i] = Napi::String::New(env, manifest.metadata.authors[i]);
    }

    module_info.DefineProperty(Napi::PropertyDescriptor::Value("authors", authors, napi_enumerable));

    return module_info;
}

Napi::Value EverestModule::say_hello(const Napi::CallbackInfo& info) {
    auto env = info.Env();

    const auto& config = this->peer->say_hello();

    Napi::Object config_object = Napi::Object::New(env);

    // FIXME (aw): all of this could be moved to conversion unit
    config_object.DefineProperty(Napi::PropertyDescriptor::Value(
        "config_sets", ev_value_to_napi_value(env, config.get_config_sets()), napi_enumerable));

    Napi::Object cxns_object = Napi::Object::New(env);

    const auto& requiremnts = this->peer->get_module().manifest.requirements;
    for (const auto& req_it : requiremnts) {
        const auto& req_id = req_it.first;
        const auto& fulfillments = config.get_fulfillments(req_id);
        auto fulfillments_napi = Napi::Array::New(env, fulfillments.size());

        for (size_t i = 0; i < fulfillments.size(); ++i) {
            auto fulfillment_object = Napi::Object::New(env);
            fulfillment_object.DefineProperties({
                Napi::PropertyDescriptor::Value("module_id", Napi::String::New(env, fulfillments[i].module_id),
                                                napi_enumerable),
                Napi::PropertyDescriptor::Value(
                    "implementation_id", Napi::String::New(env, fulfillments[i].implementation_id), napi_enumerable),
            });
            fulfillments_napi[i] = fulfillment_object;
        }

        cxns_object.DefineProperty(Napi::PropertyDescriptor::Value(req_id, fulfillments_napi, napi_enumerable));
    }

    config_object.DefineProperty(Napi::PropertyDescriptor::Value("connections", cxns_object, napi_enumerable));

    return config_object;
}

Napi::Value EverestModule::init_done(const Napi::CallbackInfo& info) {
    this->peer->init_done();
    return info.Env().Null();
}

Napi::Value EverestModule::call_command(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    const auto fulfillment = napi_object_to_fulfillment(info[0].As<Napi::Object>());
    const auto command_name = info[1].As<Napi::String>();
    const auto args = napi_value_to_ev_value(info[2]);

    const auto result = this->peer->call_command(fulfillment, command_name, args);

    return ev_value_to_napi_value(env, result);
}

Napi::Value EverestModule::publish_variable(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    const std::string implementation_id = info[0].As<Napi::String>();
    const std::string variable_name = info[1].As<Napi::String>();
    const auto value = napi_value_to_ev_value(info[2]);

    this->peer->publish_variable(implementation_id, variable_name, value);

    return env.Null();
}

Napi::Value EverestModule::implement_command(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    const auto implementation_id = info[0].As<Napi::String>();
    const auto command_name = info[1].As<Napi::String>();
    const auto handler = info[2].As<Napi::Function>();

    const auto topic = everest::utils::get_command_topic(this->peer->get_module().id, implementation_id, command_name);

    this->cmd_implementations.emplace_front(topic, env, handler);

    auto& cmd_impl = this->cmd_implementations.front();

    this->peer->implement_command(implementation_id, command_name,
                                  [&cmd_impl](const everest::Arguments& args) { return cmd_impl.dispatch(args); });

    return env.Null();
}

Napi::Value EverestModule::subscribe_variable(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    const auto fulfillment = napi_object_to_fulfillment(info[0].As<Napi::Object>());
    const auto variable_name = info[1].As<Napi::String>();
    const auto handler = info[2].As<Napi::Function>();

    const auto topic =
        everest::utils::get_variable_topic(fulfillment.module_id, fulfillment.implementation_id, variable_name);

    this->var_subscriptions.emplace_front(topic, env, handler);
    auto& var_sub = this->var_subscriptions.front();

    auto unsubscribe = this->peer->subscribe_variable(
        fulfillment, variable_name, [&var_sub](const everest::Value& data) { var_sub.dispatch(data); });

    return Napi::Function::New(
        env,
        [unsubscribe](const Napi::CallbackInfo& info) -> Napi::Value {
            unsubscribe();
            // FIXME (aw): How can we discover if we got deleted?
            return info.Env().Null();
        },
        "var_unsubscribe");
}

Napi::Value EverestModule::mqtt_publish(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    const std::string topic = info[0].As<Napi::String>();
    const std::string data = info[1].As<Napi::String>();

    this->peer->mqtt_publish(topic, data);

    return env.Null();
}

Napi::Value EverestModule::mqtt_subscribe(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    const std::string topic = info[0].As<Napi::String>();
    const auto handler = info[1].As<Napi::Function>();

    this->mqtt_subscriptions.emplace_front(topic, env, handler);
    auto& mqtt_sub = this->mqtt_subscriptions.front();

    auto unsubscribe =
        this->peer->mqtt_subscribe(topic, [&mqtt_sub](const std::string& data) { mqtt_sub.dispatch(data); });

    return Napi::Function::New(
        env,
        [unsubscribe](const Napi::CallbackInfo& info) -> Napi::Value {
            unsubscribe();
            // FIXME (aw): How can we discover if we got deleted?
            return info.Env().Null();
        },
        "mqtt_unsubscribe");
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    EverestModule::Init(env, exports);
    init_logger(env, exports);
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
