// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#include "subprocess.hpp"

#include <exception>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

#include <fmt/format.h>

bool SubprocessHandle::is_child() const {
    return this->pid == 0;
}

void SubprocessHandle::send_error_and_exit(const std::string& message) {
    // FIXME (aw): howto do asserts?
    assert(pid == 0);

    write(fd, message.c_str(), std::min(message.size(), MAX_PIPE_MESSAGE_SIZE - 1));
    close(fd);
    _exit(EXIT_FAILURE);
}

// FIXME (aw): this function should be callable only once
pid_t SubprocessHandle::check_child_executed() {
    assert(pid != 0);

    std::string message(MAX_PIPE_MESSAGE_SIZE, 0);

    auto retval = read(fd, message.data(), MAX_PIPE_MESSAGE_SIZE);
    if (retval == -1) {
        throw std::runtime_error(fmt::format(
            "Failed to communicate via pipe with forked child process. Syscall to read() failed ({}), exiting",
            strerror(errno)));
    } else if (retval > 0) {
        throw std::runtime_error(fmt::format("Forked child process did not complete exec():\n{}", message.c_str()));
    }

    close(fd);
    return pid;
}

SubprocessHandle SubprocessHandle::get_new(bool set_pdeathsig) {
    int pipefd[2];

    if (pipe2(pipefd, O_CLOEXEC | O_DIRECT)) {
        throw std::runtime_error(fmt::format("Syscall pipe2() failed ({}), exiting", strerror(errno)));
    }

    const auto reading_end_fd = pipefd[0];
    const auto writing_end_fd = pipefd[1];

    const auto parent_pid = getpid();

    pid_t pid = fork();

    if (pid == -1) {
        throw std::runtime_error(fmt::format("Syscall fork() failed ({}), exiting", strerror(errno)));
    }

    if (pid == 0) {
        // close read end in child
        close(reading_end_fd);

        SubprocessHandle handle{writing_end_fd, pid};

        if (set_pdeathsig) {
            // FIXME (aw): how does the the forked process does cleanup when receiving PARENT_DIED_SIGNAL compared to
            //             _exit() before exec() has been called?
            if (prctl(PR_SET_PDEATHSIG, PARENT_DIED_SIGNAL)) {
                handle.send_error_and_exit(fmt::format("Syscall prctl() failed ({}), exiting", strerror(errno)));
            }

            if (getppid() != parent_pid) {
                // kill ourself, with the same handler as we would have
                // happened when the parent process died
                kill(getpid(), PARENT_DIED_SIGNAL);
            }
        }

        return handle;
    } else {
        close(writing_end_fd);
        return {reading_end_fd, pid};
    }
}
