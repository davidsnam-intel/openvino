// Copyright (C) 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "cldnn/runtime/event.hpp"
#include "cldnn/runtime/stream.hpp"
#include "ocl_common.hpp"
#include "ocl_engine.hpp"

#include <memory>
#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

namespace cldnn {
namespace ocl {

class events_pool;

class ocl_stream : public stream {
public:
    const queue_type& queue() const { return _command_queue; }

    explicit ocl_stream(const ocl_engine& engine);
    ocl_stream(ocl_stream&& other)
        : _engine(other._engine),
          _command_queue(other._command_queue),
          _queue_counter(other._queue_counter.load()),
          _last_barrier(other._last_barrier.load()),
          _events_pool(std::move(other._events_pool)),
          _last_barrier_ev(other._last_barrier_ev),
          _output_event(other._output_event) {}

    // ocl_stream& operator=(ocl_stream&& other) {
    //     if (this != &other) {
    //         _engine = std::move(other._engine);
    //         _command_queue = std::move(other._command_queue);
    //         _queue_counter = std::move(other._queue_counter.load());
    //         _last_barrier = std::move(other._last_barrier.load());
    //         _events_pool = std::move(std::move(other._events_pool));
    //         _last_barrier_ev = std::move(other._last_barrier_ev);
    //         _output_event = std::move(other._output_event);
    //     }
    //     return *this;
    // }

    ~ocl_stream() = default;

    void sync_events(std::vector<event::ptr> const& deps, bool is_output_event = false) override;
    void release_pending_memory();
    void flush() override;
    void finish() override;

    void set_output_event(bool out_event) { _output_event = out_event; }

    void set_arguments(kernel& kernel, const kernel_arguments_desc& args_desc, const kernel_arguments_data& args) override;
    event::ptr enqueue_kernel(kernel& kernel,
                              const kernel_arguments_desc& args_desc,
                              const kernel_arguments_data& args,
                              std::vector<event::ptr> const& deps,
                              bool is_output_event = false) override;
    event::ptr enqueue_marker(std::vector<event::ptr> const& deps, bool is_output_event) override;
    event::ptr group_events(std::vector<event::ptr> const& deps) override;
    void wait_for_events(const std::vector<event::ptr>& events) override;
    void enqueue_barrier() override;
    void reset_events() override;
    event::ptr create_user_event(bool set) override;
    event::ptr create_base_event() override;
    void release_events_pool() override;

    queue_type get_queue() const { return _command_queue; }

private:
    const ocl_engine& _engine;
    queue_type _command_queue;
    std::atomic<uint64_t> _queue_counter{0};
    std::atomic<uint64_t> _last_barrier{0};
    std::shared_ptr<events_pool> _events_pool;
    cl::Event _last_barrier_ev;
    bool _output_event = false;
};

}  // namespace ocl
}  // namespace cldnn
