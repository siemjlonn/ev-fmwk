// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include "manager.hpp"

#include <everest/logging/logging.hpp>
#include <everest/utils/mqtt_io_mqttc.hpp>

#include <fmt/color.h>
#include <fmt/format.h>

#include "subprocess.hpp"

const auto TERMINAL_STYLE_ERROR = fmt::emphasis::bold | fg(fmt::terminal_color::red);
const auto TERMINAL_STYLE_OK = fmt::emphasis::bold | fg(fmt::terminal_color::green);

static auto spawn_module(std::string module_id, const std::string& module_binary, std::string log_config_path) {
    auto handle = SubprocessHandle::get_new();

    if (handle.is_child()) {
        char* const argv[] = {
            {module_id.data()},
            {module_id.data()},
            {log_config_path.data()},
            {NULL},
        };

        execv(module_binary.c_str(), argv);

        // exec failed
        handle.send_error_and_exit(fmt::format("Syscall to execv() with \"{} {}\" failed ({})", module_binary,
                                               fmt::join(argv, argv + 1, " "), strerror(errno)));
    }

    return handle.check_child_executed();
}

// FIXME (aw): this should be refactored to the subprocess module
static void teardown_module_subprocesses(Manager::ModuleHandleMap& modules_handles) {
    // NOTE (aw): this function assumes, that it has exclusive access to the ModuleHandleMap!
    for (auto module_it : modules_handles) {
        const auto pid = module_it.second.pid;
        const auto module_id = module_it.first;

        // FIXME (aw): this should be encoded into the ModuleState
        if (pid == -1) {
            continue;
        }

        auto retval = kill(pid, SIGTERM);
        // FIXME (aw): supply errno strings
        if (retval != 0) {
            EVLOG(critical) << fmt::format("SIGTERM of module '{}' with pid '{}' {}: {}. Escalating to SIGKILL",
                                           module_id, pid, fmt::format(TERMINAL_STYLE_ERROR, "failed"), retval);
            retval = kill(pid, SIGKILL);
            if (retval != 0) {
                EVLOG(critical) << fmt::format("SIGKILL of module '{}' with pid '{}' {}: {}", module_id, pid,
                                               fmt::format(TERMINAL_STYLE_ERROR, "failed"), retval);
            } else {
                EVLOG(info) << fmt::format("SIGKILL of module '{}' with pid '{}' {}: {}", module_id, pid,
                                           fmt::format(TERMINAL_STYLE_OK, "succeeded"));
            }
        } else {
            EVLOG(info) << fmt::format("SIGTERM of module '{}' with pid '{}': {}", module_id, pid,
                                       fmt::format(TERMINAL_STYLE_OK, "succeeded"));
        }
    }
}

Manager::Manager(everest::Config config) : config(std::move(config)) {
    for (const auto& module : this->config.get_modules()) {
        this->module_handles[module.first].state = ModuleState::NOT_STARTED;
    }
}

void Manager::run(const RuntimeEnvironment& rs) {
    everest::mqtt::MQTTC_IO mqtt_io(everest::mqtt::get_default_server_address());
    everest::Peer mngr("manager", mqtt_io);

    mngr.implement_command(
        "", "say_hello", [&mngr, this](const everest::Arguments& args) { return this->handle_say_hello(mngr, args); });

    mngr.implement_command(
        "", "init_done", [&mngr, this](const everest::Arguments& args) { return this->handle_init_done(mngr, args); });

    {
        const std::lock_guard<std::mutex> lock(this->mutex);
        for (auto& module_it : this->module_handles) {
            auto& module_handle = module_it.second;
            const auto& module_id = module_it.first;
            const auto& module_type = this->config.get_modules().at(module_id).type;

            const auto module_binary = fmt::format("{}/{}/{}", rs.modules_path.string(), module_type, module_type);

            const auto spawn_this_module =
                (rs.standalone_modules.end() == std::find_if(rs.standalone_modules.begin(), rs.standalone_modules.end(),
                                                             [&module_id](const auto& id) { return id == module_id; }));

            if (spawn_this_module) {
                module_handle.pid = spawn_module(module_id, module_binary, rs.logging_config_path.string());
            }

            module_handle.state = ModuleState::NOT_SEEN;
        }
    }

    while (true) {
        this->check_subprocesses();
        mqtt_io.sync(50);
    }
}

void Manager::check_subprocesses() {
    // check if anyone died
    int wstatus;

    // FIXME (aw): this would also fire if a process got stopped or signaled
    auto pid = waitpid(-1, &wstatus, WNOHANG);

    if (pid == 0) {
        // nothing new from our child process
        return;
    } else if (pid == -1) {
        // FIXME (aw): the checking for ECHILD is buggy, we should probably do that only once?
        if (errno == ECHILD) {
            return;
        }
        throw std::runtime_error(fmt::format("Syscall to waitpid() failed ({})", strerror(errno)));
    } else {
        const std::lock_guard<std::mutex> lock(this->mutex);
        auto module_it =
            std::find_if(this->module_handles.begin(), this->module_handles.end(),
                         [pid](const std::pair<std::string, ModuleHandle>& it) { return it.second.pid == pid; });

        if (module_it == this->module_handles.end()) {
            throw std::runtime_error(fmt::format("Unkown child width pid ({}) died.", pid));
        }

        auto& module_handle = module_it->second;
        EVLOG(info) << fmt::format("Module '{}' with pid '{}' exited with status: {}", module_it->first,
                                   module_handle.pid, wstatus);

        // FIXME (aw): instead of setting the pid to -1, so it won't get killed again, we should set the state to
        // appropriate value
        module_handle.pid = -1;

        // one of our started module processes died -> kill all process we started

        if (!this->module_teardown_started) {
            teardown_module_subprocesses(this->module_handles);
            this->module_teardown_started = true;
        }
    }
}

everest::Arguments Manager::handle_say_hello(everest::Peer& mngr, const everest::Arguments& args) {
    // expecting { "module_id": }
    const std::string module_id = args.at("module_id");

    const std::lock_guard<std::mutex> lock(this->mutex);

    auto module_it = module_handles.find(module_id);
    if (module_it == module_handles.end()) {
        return {{"error", "Sorry, I do not know you."}};
    }

    auto& module_state = module_it->second.state;

    if (module_state == ModuleState::NOT_STARTED) {
        return {{"error", "Do you time travel?"}};
    }

    if (module_state == ModuleState::NOT_SEEN) {
        module_state = ModuleState::SAID_HELLO;
        return this->config.get_modules().at(module_id).config;
    }

    return {{"error", "You already said hello."}};
}

everest::Value Manager::handle_init_done(everest::Peer& mngr, const everest::Arguments& args) {
    // expecting { "module_id": }
    const std::string module_id = args.at("module_id");

    std::unique_lock<std::mutex> lock(this->mutex);

    auto module_it = module_handles.find(module_id);
    if (module_it == module_handles.end()) {
        return {{"error", "Sorry, I do not know you."}};
    }

    auto& module_state = module_it->second.state;

    if (module_state == ModuleState::SAID_HELLO) {
        module_state = ModuleState::INIT_FINISHED;
        this->number_of_initialized_modules += 1;

        if (this->number_of_initialized_modules == this->module_handles.size()) {
            lock.unlock();
            // publish all the connections here
            mngr.publish_variable("", "ready", nullptr);
        }

        return nullptr;
    }

    return {{"error", "I did not expect you tell me that you are done with init."}};
}
