// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef SRC_MANAGER_MANAGER_HPP
#define SRC_MANAGER_MANAGER_HPP

#include <mutex>
#include <unordered_map>

#include <everest/peer.hpp>
#include <everest/utils/config.hpp>

#include "runtime_env.hpp"

enum class ModuleState
{
    NOT_STARTED,
    NOT_SEEN,
    SAID_HELLO,
    INIT_FINISHED,
};

struct ModuleHandle {
    ModuleState state{ModuleState::NOT_STARTED};
    pid_t pid{-1}; // FIXME (aw): this could also be the subprocess handle, given a better interface
};

class Manager {
public:
    using ModuleHandleMap = std::unordered_map<std::string, ModuleHandle>;
    Manager(everest::Config config);
    void run(const RuntimeEnvironment& rs);

    everest::Value handle_say_hello(everest::Peer& mngr, const everest::Arguments& args);
    everest::Value handle_init_done(everest::Peer& mngr, const everest::Arguments& args);

private:
    void check_subprocesses();
    bool module_teardown_started{false};

    everest::Config config;
    std::mutex mutex;

    size_t number_of_initialized_modules{0};

    ModuleHandleMap module_handles;
};

#endif // SRC_MANAGER_MANAGER_HPP
