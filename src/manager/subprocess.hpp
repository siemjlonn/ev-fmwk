// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef SRC_MANAGER_SUB_PROCESS_HPP
#define SRC_MANAGER_SUB_PROCESS_HPP

#include <string>

#include <sys/types.h>
#include <sys/wait.h>

const auto PARENT_DIED_SIGNAL = SIGTERM;

class SubprocessHandle {
public:
    static SubprocessHandle get_new(bool set_pdeathsig = true);

    bool is_child() const;
    void send_error_and_exit(const std::string& message);

    // FIXME (aw): this function should be callable only once
    pid_t check_child_executed();

private:
    const size_t MAX_PIPE_MESSAGE_SIZE = 1024;
    SubprocessHandle(int fd, pid_t pid) : fd(fd), pid(pid){};
    int fd{};
    pid_t pid{};
};

#endif // SRC_MANAGER_SUB_PROCESS_HPP
