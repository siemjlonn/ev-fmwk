// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include "testing_helpers.hpp"
#include <catch2/catch.hpp>

#include <everest/utils/config.hpp>

class ConfigLoaderMock : public everest::ModuleManifestLoaderBase {
private:
    std::string get_raw_manifest(const std::string& module_type) const override final {
        return read_resource_file("config/" + module_type + ".json");
    }
};

SCENARIO("Check config parser", "[!throws]") {
    GIVEN("A mocked config loader") {
        ConfigLoaderMock cfg_loader;
        WHEN("Given a valid module manifests") {
            nlohmann::json config = read_json_file("schema/valid_config_0.json");

            everest::Config(cfg_loader, config);
            THEN("The config should load successfully") {
                // manifest = read_json_file("schema/invalid_manifest_0.json");
                // REQUIRE_THROWS(everest::Config(cfg_loader, config));
            }
        }
    }
}
