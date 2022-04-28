// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_UTILS_THREAD_HPP
#define EVEREST_UTILS_THREAD_HPP

#include <chrono>
#include <future>
#include <thread>

namespace everest {
class Thread {
public:
    Thread();
    ~Thread();

    bool shouldExit();
    void operator=(std::thread&&);

private:
    std::thread handle;
    std::promise<void> exitSignal;
    std::future<void> exitFuture;
};
} // namespace everest

#endif // EVEREST_UTILS_THREAD_HPP
