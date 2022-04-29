// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef EVEREST_UTILS_WORK_CONTEXT_HPP
#define EVEREST_UTILS_WORK_CONTEXT_HPP

#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

namespace everest::utils {

// FIXME (aw): how should an already running message dispatch behave if
// one of its handlers get removed?  How should we behave if a message
// gets added, but there are no handlers for it at all?

template <typename MessageType>
class MessageWorker {
    using HandlerType = std::function<void(const MessageType& message)>;
    using HandlerToken = typename std::list<HandlerType>::const_iterator;

private:
    struct Topic {
        std::list<HandlerType> handlers{};
        std::vector<HandlerType> cached_handlers{};
        bool handler_cache_is_dirty{false};
    };

    std::mutex mutex;
    std::unordered_map<std::string, Topic> topics;
    std::queue<std::pair<std::string, MessageType>> work_to_do;
    std::condition_variable cv;
    std::thread worker_thread;
    bool keep_on_running{true};

    std::vector<HandlerType> const& get_cached_handlers(Topic& topic) {
        if (topic.handler_cache_is_dirty) {
            topic.cached_handlers.clear();
            topic.cached_handlers.insert(topic.cached_handlers.begin(), topic.handlers.begin(), topic.handlers.end());
            topic.handler_cache_is_dirty = false;
        }

        return topic.cached_handlers;
    }

    class LockedAccess {
    public:
        void add_work(const std::string& topic_id, MessageType&& message) {
            // NOTE (aw): it is very unlikely, that there are not
            // handlers registered for the topic, therefore we add the
            // message here and drop it later on, if we see that there
            // are not handlers - BUT: this might lead to undesired
            // behaviour because a later added handler could get data,
            // which came in before

            worker->work_to_do.emplace(topic_id, std::move(message));
            worker->cv.notify_all();
        };

        std::pair<bool, HandlerToken> add_handler(const std::string& topic_id, const HandlerType& handler) {
            auto& topic = worker->topics[topic_id];
            bool handler_list_was_empty = (topic.handlers.size() == 0);
            topic.handlers.emplace_back(handler);
            topic.handler_cache_is_dirty = true;
            return std::make_pair(handler_list_was_empty, std::prev(topic.handlers.end()));
        }

        bool remove_handler(const std::string& topic_id, const HandlerToken& handler_token) {
            auto& topic = worker->topics.at(topic_id);

            topic.handlers.erase(handler_token);
            topic.handler_cache_is_dirty = true;
            return topic.handlers.empty();
        }

        size_t handler_count(const std::string& topic_id) const {
            const auto topic_it = worker->topics.find(topic_id);
            if (topic_it == worker->topics.end()) {
                return 0;
            }

            return topic_it->second.handlers.size();
        }

        LockedAccess(MessageWorker* worker) : worker(worker), lock(worker->mutex) {
        }

    private:
        MessageWorker* worker;
        const std::lock_guard<std::mutex> lock;
    };

public:
    MessageWorker() {
        this->worker_thread = std::thread([this]() {
            while (true) {
                std::unique_lock<std::mutex> lock(this->mutex);
                while (this->keep_on_running && this->work_to_do.empty()) {
                    this->cv.wait(lock);
                }

                if (this->keep_on_running == false) {
                    return;
                }

                // messages must be available
                auto work = std::move(this->work_to_do.front());
                this->work_to_do.pop();

                const auto topic_it = this->topics.find(work.first);

                if (topic_it == this->topics.end()) {
                    // FIXME (aw): should we error here?
                    continue;
                }

                auto& handlers = get_cached_handlers(topic_it->second);

                lock.unlock();

                for (auto& handler : handlers) {
                    handler(work.second);
                    // FIXME (aw): should we early quit if thread should terminate?
                }
            }
        });
    }

    ~MessageWorker() {
        {
            // FIXME (aw): is this lock necessary?
            const std::lock_guard<std::mutex> lock(mutex);
            this->keep_on_running = false;
        }

        cv.notify_all();

        if (worker_thread.joinable()) {
            worker_thread.join();
        }
    }

    LockedAccess get_locked_access() {
        return this;
    }
};

template <typename MessageType> class RegisteredHandlers {
public:
    MessageWorker<MessageType>& get(const std::string& key) {
        const std::lock_guard<std::mutex> lock(this->mutex);
        return this->map[key];
    }
    MessageWorker<MessageType>* find(const std::string& key) {
        const std::lock_guard<std::mutex> lock(this->mutex);
        const auto& worker_it = this->map.find(key);
        return (worker_it == this->map.end()) ? nullptr : &worker_it->second;
    }

private:
    // NOTE (aw): in most case, there will only be few keys (< 10) so a
    // vector might be even faster - but then the resize behaviour must
    // be treated carefully
    std::unordered_map<std::string, MessageWorker<MessageType>> map{};
    std::mutex mutex{};
};

} // namespace everest::utils

#endif // EVEREST_UTILS_WORK_CONTEXT_HPP
