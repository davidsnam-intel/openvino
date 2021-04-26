/*
// Copyright (c) 2019-2021 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

///////////////////////////////////////////////////////////////////////////////////////////////////

#include "ocl_stream.hpp"
#include "ocl_base_event.hpp"
#include "ocl_user_event.hpp"
#include "ocl_command_queues_builder.hpp"
#include "ocl_events_pool.hpp"
#include "ocl_kernel.hpp"
#include "ocl_common.hpp"

#include <cassert>
#include <iomanip>
#include <ios>

#include <fstream>
#include <thread>
#include <string>
#include <vector>
#include <memory>

// NOTE: Due to buggy scope transition of warnings we need to disable warning in place of use/instantation
//       of some types (even though we already disabled them in scope of definition of these types).
//       Moreover this warning is pretty much now only for annoyance: it is generated due to lack
//       of proper support for mangling of custom GCC attributes into type name (usually when used
//       with templates, even from standard library).
#if defined __GNUC__ && __GNUC__ >= 6
#pragma GCC diagnostic ignored "-Wignored-attributes"
#endif

namespace cldnn {
namespace gpu {

namespace {
inline cl::NDRange toNDRange(const std::vector<size_t>& v) {
    switch (v.size()) {
        case 1:
            return cl::NDRange(v[0]);
        case 2:
            return cl::NDRange(v[0], v[1]);
        case 3:
            return cl::NDRange(v[0], v[1], v[2]);
        default:
            return cl::NullRange;
    }
}

void set_arguments_impl(kernel_type& kernel,
                        const arguments_desc& args,
                        const kernel_arguments_data& data) {
    using args_t = argument_desc::Types;
    using scalar_t = scalar_desc::Types;
    for (uint32_t i = 0; i < static_cast<uint32_t>(args.size()); i++) {
        cl_int status = CL_INVALID_ARG_VALUE;
        switch (args[i].t) {
            case args_t::INPUT:
                if (args[i].index < data.inputs.size() && data.inputs[args[i].index]) {
                    const auto& input_mem = data.inputs[args[i].index];
                    if (input_mem) {
                        if (input_mem->get_layout().format.is_image_2d())
                            status = kernel.setArg(i, std::dynamic_pointer_cast<const gpu::gpu_image2d>(input_mem)->get_buffer());
                        else if (memory_capabilities::is_usm_type(input_mem->get_allocation_type()))
                            status = kernel.setArgUsm(i, std::dynamic_pointer_cast<const gpu::gpu_usm>(input_mem)->get_buffer());
                        else
                            status = kernel.setArg(i, std::dynamic_pointer_cast<const gpu::gpu_buffer>(input_mem)->get_buffer());
                    }
                }
                break;
            case args_t::INPUT_OF_FUSED_PRIMITIVE:
                if (args[i].index < data.fused_op_inputs.size() && data.fused_op_inputs[args[i].index]) {
                    const auto& input_mem = data.fused_op_inputs[args[i].index];
                    if (input_mem) {
                        if (memory_capabilities::is_usm_type(input_mem->get_allocation_type()))
                            status = kernel.setArgUsm(i, std::dynamic_pointer_cast<const gpu::gpu_usm>(input_mem)->get_buffer());
                        else
                            status = kernel.setArg(i, std::dynamic_pointer_cast<const gpu::gpu_buffer>(input_mem)->get_buffer());
                    }
                }
                break;
            case args_t::INTERNAL_BUFFER:
                if (args[i].index < data.intermediates.size() && data.intermediates[args[i].index]) {
                    const auto& input_mem = data.intermediates[args[i].index];
                    if (input_mem) {
                        if (memory_capabilities::is_usm_type(input_mem->get_allocation_type()))
                            status = kernel.setArgUsm(i, std::dynamic_pointer_cast<const gpu::gpu_usm>(input_mem)->get_buffer());
                        else
                            status = kernel.setArg(i, std::dynamic_pointer_cast<const gpu::gpu_buffer>(input_mem)->get_buffer());
                    }
                }
                break;
            case args_t::OUTPUT:
                if (data.output) {
                     if (data.output->get_layout().format.is_image_2d())
                        status = kernel.setArg(i, std::dynamic_pointer_cast<const gpu::gpu_image2d>(data.output)->get_buffer());
                     else if (memory_capabilities::is_usm_type(data.output->get_allocation_type()))
                         status = kernel.setArgUsm(i, std::dynamic_pointer_cast<const gpu::gpu_usm>(data.output)->get_buffer());
                     else
                        status = kernel.setArg(i, std::dynamic_pointer_cast<const gpu::gpu_buffer>(data.output)->get_buffer());
                }
                break;
            case args_t::WEIGHTS:
                if (data.weights) {
                    if (data.weights->get_layout().format.is_image_2d())
                        status = kernel.setArg(i, std::dynamic_pointer_cast<const gpu::gpu_image2d>(data.weights)->get_buffer());
                    else if (memory_capabilities::is_usm_type(data.weights->get_allocation_type()))
                        status = kernel.setArgUsm(i, std::dynamic_pointer_cast<const gpu::gpu_usm>(data.weights)->get_buffer());
                    else
                        status = kernel.setArg(i, std::dynamic_pointer_cast<const gpu::gpu_buffer>(data.weights)->get_buffer());
                }
                break;
            case args_t::BIAS:
                if (data.bias) {
                    if (memory_capabilities::is_usm_type(data.bias->get_allocation_type()))
                        status = kernel.setArgUsm(i, std::dynamic_pointer_cast<const gpu::gpu_usm>(data.bias)->get_buffer());
                    else
                        status = kernel.setArg(i, std::dynamic_pointer_cast<const gpu::gpu_buffer>(data.bias)->get_buffer());
                }
                break;
            case args_t::WEIGHTS_ZERO_POINTS:
                if (data.weights_zero_points) {
                    if (memory_capabilities::is_usm_type(data.weights_zero_points->get_allocation_type()))
                        status = kernel.setArgUsm(
                            i,
                            std::dynamic_pointer_cast<const gpu::gpu_usm>(data.weights_zero_points)->get_buffer());
                    else
                        status = kernel.setArg(
                            i,
                            std::dynamic_pointer_cast<const gpu::gpu_buffer>(data.weights_zero_points)->get_buffer());
                }
                break;
            case args_t::ACTIVATIONS_ZERO_POINTS:
                if (data.activations_zero_points) {
                    if (memory_capabilities::is_usm_type(data.activations_zero_points->get_allocation_type()))
                        status = kernel.setArgUsm(
                            i,
                            std::dynamic_pointer_cast<const gpu::gpu_usm>(data.activations_zero_points)->get_buffer());
                    else
                        status = kernel.setArg(
                            i,
                            std::dynamic_pointer_cast<const gpu::gpu_buffer>(data.activations_zero_points)->get_buffer());
                }
                break;
            case args_t::COMPENSATION:
                if (data.compensation) {
                    if (memory_capabilities::is_usm_type(data.compensation->get_allocation_type()))
                        status = kernel.setArgUsm(
                                i,
                                std::dynamic_pointer_cast<const gpu::gpu_usm>(data.compensation)->get_buffer());
                    else
                        status = kernel.setArg(
                                 i,
                                 std::dynamic_pointer_cast<const gpu::gpu_buffer>(data.compensation)->get_buffer());
                }
                break;
            case args_t::SCALE_TABLE:
                if (data.scale_table) {
                    if (memory_capabilities::is_usm_type(data.scale_table->get_allocation_type()))
                        status = kernel.setArgUsm(i, std::dynamic_pointer_cast<const gpu::gpu_usm>(data.scale_table)->get_buffer());
                    else
                        status = kernel.setArg(i, std::dynamic_pointer_cast<const gpu::gpu_buffer>(data.scale_table)->get_buffer());
                }
                break;
            case args_t::SLOPE:
                if (data.slope) {
                    if (memory_capabilities::is_usm_type(data.slope->get_allocation_type()))
                        status = kernel.setArgUsm(i, std::dynamic_pointer_cast<const gpu::gpu_usm>(data.slope)->get_buffer());
                    else
                        status = kernel.setArg(i, std::dynamic_pointer_cast<const gpu::gpu_buffer>(data.slope)->get_buffer());
                }
                break;
            case args_t::SPLIT:
                status = kernel.setArg(i, data.split);
                break;
            case args_t::SCALAR:
                if (data.scalars && args[i].index < data.scalars->size()) {
                    const auto& scalar = (*data.scalars)[args[i].index];
                    switch (scalar.t) {
                        case scalar_t::UINT8:
                            status = kernel.setArg(i, scalar.v.u8);
                            break;
                        case scalar_t::UINT16:
                            status = kernel.setArg(i, scalar.v.u16);
                            break;
                        case scalar_t::UINT32:
                            status = kernel.setArg(i, scalar.v.u32);
                            break;
                        case scalar_t::UINT64:
                            status = kernel.setArg(i, scalar.v.u64);
                            break;
                        case scalar_t::INT8:
                            status = kernel.setArg(i, scalar.v.s8);
                            break;
                        case scalar_t::INT16:
                            status = kernel.setArg(i, scalar.v.s16);
                            break;
                        case scalar_t::INT32:
                            status = kernel.setArg(i, scalar.v.s32);
                            break;
                        case scalar_t::INT64:
                            status = kernel.setArg(i, scalar.v.s64);
                            break;
                        case scalar_t::FLOAT32:
                            status = kernel.setArg(i, scalar.v.f32);
                            break;
                        case scalar_t::FLOAT64:
                            status = kernel.setArg(i, scalar.v.f64);
                            break;
                        default:
                            break;
                    }
                }
                break;
            case args_t::RECURRENT:  // RNN/LSTM/GRU layers
                if (data.recurrent) {
                    if (data.recurrent->get_layout().format.is_image_2d())
                        status = kernel.setArg(i, dynamic_cast<const gpu::gpu_image2d&>(*data.recurrent).get_buffer());
                    else if (memory_capabilities::is_usm_type(data.recurrent->get_allocation_type()))
                        status = kernel.setArgUsm(i, dynamic_cast<const gpu::gpu_usm&>(*data.recurrent).get_buffer());
                    else
                        status = kernel.setArg(i, dynamic_cast<const gpu::gpu_buffer&>(*data.recurrent).get_buffer());
                }
                break;
            case args_t::HIDDEN:  // RNN/LSTM/GRU layers
                if (data.hidden) {
                    if (data.hidden->get_layout().format.is_image_2d())
                        status = kernel.setArg(i, dynamic_cast<const gpu::gpu_image2d&>(*data.hidden).get_buffer());
                    else if (memory_capabilities::is_usm_type(data.hidden->get_allocation_type()))
                        status = kernel.setArgUsm(i, dynamic_cast<const gpu::gpu_usm&>(*data.hidden).get_buffer());
                    else
                        status = kernel.setArg(i, dynamic_cast<const gpu::gpu_buffer&>(*data.hidden).get_buffer());
                }
                break;
            case args_t::CELL:  // LSTMlayers
                if (data.cell) {
                    if (data.cell->get_layout().format.is_image_2d())
                        status = kernel.setArg(i, dynamic_cast<const gpu::gpu_image2d&>(*data.cell).get_buffer());
                    else if (memory_capabilities::is_usm_type(data.cell->get_allocation_type()))
                        status = kernel.setArgUsm(i, dynamic_cast<const gpu::gpu_usm&>(*data.cell).get_buffer());
                    else
                        status = kernel.setArg(i, dynamic_cast<const gpu::gpu_buffer&>(*data.cell).get_buffer());
                }
                break;
            default:
                break;
        }

        if (status != CL_SUCCESS) {
            throw std::runtime_error("Error set arg " + std::to_string(i) + ", error code: " + std::to_string(status) + "\n");
        }
    }
}
}  // namespace

ocl_stream::ocl_stream(const ocl_engine& engine) : _engine(engine) {
    auto context = engine.get_cl_context();
    auto device = engine.get_cl_device();
    auto config = engine.configuration();
    gpu::command_queues_builder queue_builder(context, device);
    queue_builder.set_profiling(config.enable_profiling);
    queue_builder.set_out_of_order((config.use_out_of_order_queue));

    bool priorty_extensions = engine.extension_supported("cl_khr_priority_hints") && engine.extension_supported("cl_khr_create_command_queue");
    queue_builder.set_priority_mode(config.priority_mode, priorty_extensions);

    bool throttle_extensions = engine.extension_supported("cl_khr_throttle_hints") && engine.extension_supported("cl_khr_create_command_queue");
    queue_builder.set_throttle_mode(config.throttle_mode, throttle_extensions);

    queue_builder.build();

    _command_queue = queue_builder.queue();
    _events_pool.reset(new events_pool());
}

void ocl_stream::set_arguments(kernel& kernel, const kernel_arguments_desc& args_desc, const kernel_arguments_data& args) {
    static std::mutex m;
    std::lock_guard<std::mutex> guard(m);

    auto& ocl_kernel = dynamic_cast<gpu::ocl_kernel&>(kernel);

    auto kern = ocl_kernel.get_handle();

    try {
        set_arguments_impl(kern, args_desc.arguments, args);
    } catch (cl::Error const& err) {
        throw ocl_error(err);
    }
}

event::ptr ocl_stream::enqueue_kernel(kernel& kernel,
                                      const kernel_arguments_desc& args_desc,
                                      const kernel_arguments_data& /* args */,
                                      std::vector<event::ptr> const& deps,
                                      bool is_output_event) {
    auto& ocl_kernel = dynamic_cast<gpu::ocl_kernel&>(kernel);

    auto kern = ocl_kernel.get_handle();
    auto global = toNDRange(args_desc.workGroups.global);
    auto local = toNDRange(args_desc.workGroups.local);
    std::vector<cl::Event> dep_events;
    auto dep_events_ptr = &dep_events;
    if (!_engine.configuration().use_out_of_order_queue) {
        for (auto& dep : deps) {
            if (auto ocl_base_ev = dynamic_cast<ocl_base_event*>(dep.get())) {
                dep_events.push_back(ocl_base_ev->get());
            }
        }
    } else {
        dep_events_ptr = nullptr;

        sync_events(deps, is_output_event);
    }

    cl::Event ret_ev;

    bool set_output_event = !_engine.configuration().use_out_of_order_queue || is_output_event || _engine.configuration().enable_profiling;

    try {
        _command_queue.enqueueNDRangeKernel(kern, cl::NullRange, global, local, dep_events_ptr, set_output_event ? &ret_ev : nullptr);
    } catch (cl::Error const& err) {
        throw ocl_error(err);
    }

    return _events_pool->get_from_base_pool(_engine.get_cl_context(), ret_ev, ++_queue_counter);
}

void ocl_stream::enqueue_barrier() {
    _command_queue.enqueueBarrierWithWaitList(nullptr, nullptr);
}

event::ptr ocl_stream::enqueue_marker(std::vector<event::ptr> const& deps, bool is_output_event) {
    if (deps.empty())
        return create_user_event(true);

    if (!_engine.configuration().use_out_of_order_queue) {
        cl::Event ret_ev;
        std::vector<cl::Event> dep_events;
        for (auto& dep : deps) {
            if (auto ocl_base_ev = dynamic_cast<ocl_base_event*>(dep.get()))
                dep_events.push_back(ocl_base_ev->get());
        }

        try {
            _command_queue.enqueueMarkerWithWaitList(&dep_events, &ret_ev);
        } catch (cl::Error const& err) {
            throw ocl_error(err);
        }

        return _events_pool->get_from_base_pool(_engine.get_cl_context(), ret_ev, ++_queue_counter);
    } else {
        sync_events(deps, is_output_event);
        return _events_pool->get_from_base_pool(_engine.get_cl_context(), _last_barrier_ev, _last_barrier);
    }
}

event::ptr ocl_stream::group_events(std::vector<event::ptr> const& deps) {
    return _events_pool->get_from_group_pool(_engine.get_cl_context(), deps);
}

event::ptr ocl_stream::create_user_event(bool set) {
    return _events_pool->get_from_user_pool(_engine.get_cl_context(), set);
}

event::ptr ocl_stream::create_base_event() {
    return _events_pool->get_from_base_pool(_engine.get_cl_context(), _last_barrier_ev, _last_barrier);
}

void ocl_stream::reset_events() { _events_pool->reset_events(); }

void ocl_stream::release_events_pool() { _events_pool.reset(); }

void ocl_stream::flush() { queue().flush(); }
void ocl_stream::finish() { queue().finish(); }

void ocl_stream::wait_for_events(const std::vector<event::ptr>& events) {
    if (events.empty())
        return;

    std::vector<cl::Event> clevents;
    for (auto& ev : events) {
        if (auto ocl_base_ev = dynamic_cast<ocl_base_event*>(ev.get()))
            clevents.push_back(ocl_base_ev->get());
    }

    try {
        cl::WaitForEvents(clevents);
    } catch (cl::Error const& err) {
        throw ocl_error(err);
    }
}

void ocl_stream::release_pending_memory() {
    // /*
    // TODO: Temp. solution, untill proper API calls from OpenCL are released.
    // */
    // void* ptr = nullptr;
    // ptr = _mm_malloc(4096, 4096);
    // queue().finish();
    // try {
    //     cl::Buffer flusher(context()->context(), CL_MEM_USE_HOST_PTR, (size_t)4096, ptr);
    //     flusher = (cl_mem) nullptr;  // clear buffer
    // } catch (...) {
    //     _mm_free(ptr);
    //     throw;
    // }
    // _mm_free(ptr);
}

void ocl_stream::sync_events(std::vector<event::ptr> const& deps, bool is_output_event) {
    bool needs_barrier = false;
    for (auto& dep : deps) {
        auto* ocl_base_ev = dynamic_cast<ocl_base_event*>(dep.get());
        if (ocl_base_ev->get_queue_stamp() > _last_barrier) {
            needs_barrier = true;
        }
    }

    if (needs_barrier) {
        try {
            if (is_output_event)
                _command_queue.enqueueBarrierWithWaitList(nullptr, &_last_barrier_ev);
            else
                _command_queue.enqueueBarrierWithWaitList(nullptr, nullptr);
        } catch (cl::Error const& err) {
            throw ocl_error(err);
        }

        _last_barrier = ++_queue_counter;
    }
}

}  // namespace gpu
}  // namespace cldnn
