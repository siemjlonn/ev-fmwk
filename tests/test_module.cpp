// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <catch2/catch.hpp>
using Catch::Matchers::Contains;
#include "testing_helpers.hpp"

#include <everest/module.hpp>
#include <everest/types.hpp>
#include <everest/utils/schema.hpp>

SCENARIO("Check module initialization", "[!throws]") {
    GIVEN("A primitive module description") {
        auto interface_intf = everest::schema::parse_interface(read_resource_file("interfaces/intf.json"));
        auto interface_intf2 = everest::schema::parse_interface(read_resource_file("interfaces/intf2.json"));

        everest::Module mod = everest::ModuleBuilder("fool", "some_hash")
                                  .add_requirement("req1", {"intf", 1, 2}, interface_intf)
                                  .add_implementation("impl1", {"intf2", {}}, interface_intf2);

        THEN("The module should contain a requirement called req1") {
            REQUIRE(mod.manifest.requirements.count("req1"));
        }

        // THEN("The module should not implement a command") {
        //     REQUIRE(mod.command_implementable("impl1", "foo_cmd") == false);
        // }

        // THEN("The module should implement a command cmd_x for implementation id impl1") {
        //     REQUIRE(mod.command_implementable("impl1", "cmd_x") == true);
        // }

        // THEN("The module should not implement a command cmd_x for unknown implementation id impl2") {
        //     REQUIRE(mod.command_implementable("impl2", "cmd_x") == false);
        // }

        // THEN("Looking up the non-existing command foo in impl1 should throw") {
        //     REQUIRE_THROWS_AS(mod.get_command_type_for_implementation("impl1", "foo"), std::runtime_error);
        //     CHECK_THROWS_WITH(mod.get_command_type_for_implementation("impl1", "foo"), Contains("not have cmd"));
        //     CHECK_THROWS_WITH(mod.get_command_type_for_implementation("impl2", "foo"), Contains("does not exist"));
        // }

        // THEN("Command cmd_x of impl1 should validate a string or number for arg1") {
        //     const auto& cmd = mod.get_command_type_for_implementation("impl1", "cmd_x");
        //     REQUIRE(cmd.arguments.at("arg1").validate("Hi there") == true);
        //     REQUIRE(everest::arguments_match_command_type({{"arg1", 2}}, cmd) == true);
        // }
    }
}
