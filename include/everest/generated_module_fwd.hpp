// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_GENERATED_MODULE_FWD_HPP
#define EVEREST_GENERATED_MODULE_FWD_HPP

#include "types.hpp"

namespace everest {

// forward declarations
struct ModulePeer;
struct Fulfillment;

namespace types {
using Object = nlohmann::json;
using Array = nlohmann::json;
using Variant = nlohmann::json;
} // namespace types

struct ImplementationContext {
    ModulePeer& module_peer;
    std::string implementation_id;
};

struct RequirementContext {
    ModulePeer& module_peer;
    const Fulfillment& fulfillment;
    std::string requirement_id;
};

struct ModuleInfo {
    std::string name;
    std::vector<std::string> authors;
    std::string license;
    std::string id;
};

template<typename BootCtxType, typename ImplListType, typename RunCtxType>
class GeneratedModuleBase {
public:
    virtual ImplListType init(BootCtxType&) = 0;
    virtual void setup(RunCtxType&) = 0;
    virtual void ready() = 0;
};

class ExternalMQTT {
public:
    ExternalMQTT(ModulePeer&);
    UnsubscriptionCallback subscribe(const std::string& topic, const MQTTSubscriptionHandler& handler);
    void publish(const std::string& topic, const std::string& data);

private:
    ModulePeer& module_peer;
};

} // namespace everest

#endif // EVEREST_GENERATED_MODULE_FWD_HPP
