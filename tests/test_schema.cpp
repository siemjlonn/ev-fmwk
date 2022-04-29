// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include <catch2/catch.hpp>

#include "testing_helpers.hpp"

#include <everest/utils/helpers.hpp>
#include <everest/utils/schema.hpp>

SCENARIO("Check schema functionality", "[!throws]") {
    GIVEN("Nothing in particular") {

        WHEN("Validating various module manifests") {
            THEN("Invalid module manifests should fail") {
                auto manifest = read_resource_file("schema/invalid_manifest_0.json");
                REQUIRE_THROWS(everest::schema::parse_module(manifest));
            }

            THEN("Valid module manifest should not fail") {
                auto manifest = read_resource_file("schema/valid_manifest_0.json");
                auto mod = everest::schema::parse_module(manifest);
                REQUIRE(mod.capabilities[1] == "nice");
                REQUIRE(mod.config_schemas.at("setting1").validate("string"));
                REQUIRE(mod.implementations.at("impl1").interface == "foobar");
                REQUIRE(mod.implementations.at("impl1").config_schemas.at("impl_setting").validate(23));
                REQUIRE(mod.requirements.at("req1").interface == "sample_interface");
                REQUIRE(mod.requirements.at("req1").is_vector());
                REQUIRE(mod.metadata.license == "http://MIT");
                REQUIRE(mod.metadata.authors.front() == "a1");
                REQUIRE_FALSE(mod.requirements.at("req2").is_vector());
            }
        }
    }

    GIVEN("The set of compiled-in schema files") {
        THEN("Their hashes should match") {
            const auto& config_schema = everest::schema::get_config_schema();
            REQUIRE(everest::utils::md5_hash(config_schema.text, config_schema.byte_size) == config_schema.md5);
            const auto& module_schema = everest::schema::get_module_schema();
            REQUIRE(everest::utils::md5_hash(module_schema.text, module_schema.byte_size) == module_schema.md5);
            const auto& interface_schema = everest::schema::get_interface_schema();
            REQUIRE(everest::utils::md5_hash(interface_schema.text, interface_schema.byte_size) == interface_schema.md5);
        }
    }
}
