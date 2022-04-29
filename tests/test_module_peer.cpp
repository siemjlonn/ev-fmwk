// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <catch2/catch.hpp>
using Catch::Matchers::Contains;

#include "testing_helpers.hpp"

#include <everest/module_peer.hpp>

SCENARIO("Check module peer functionality", "[!throws]") {
    everest::mqtt::IOBaseMock mqtt_io;
    GIVEN("A primitive module description") {
        auto interface_intf = everest::schema::parse_interface(read_resource_file("interfaces/intf.json"));
        auto interface_intf2 = everest::schema::parse_interface(read_resource_file("interfaces/intf2.json"));

        everest::Module mod = everest::ModuleBuilder("fool", "some_hash")
                                  .add_requirement("req1", {"intf", 1, 2}, interface_intf)
                                  .add_implementation("impl1", {"intf2", {}}, interface_intf2);

        everest::ModulePeer mod_peer{
            mod,
            std::make_unique<everest::mqtt::IOBaseMock>(std::move(mqtt_io)),
        };
    }
}
