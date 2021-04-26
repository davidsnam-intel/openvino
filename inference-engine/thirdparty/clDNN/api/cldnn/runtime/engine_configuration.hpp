// Copyright (C) 2018-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "utils.hpp"

#include <string>
#include <stdexcept>
#include <thread>

namespace cldnn {

/// @addtogroup cpp_api C++ API
/// @{

/// @defgroup cpp_engine Execution Engine
/// @{

/// @brief Defines available engine types
enum class engine_types : int32_t {
    ocl,
    sycl
};

/// @brief Defines available runtime types
enum class runtime_types : int32_t {
    ocl,
    l0
};

/// @brief Defines available priority mode types
enum class priority_mode_types : int16_t {
    disabled,
    low,
    med,
    high
};

/// @brief Defines available priority mode types
enum class throttle_mode_types : int16_t {
    disabled,
    low,
    med,
    high
};

/// @brief Configuration parameters for created engine.
struct engine_configuration {
    const bool enable_profiling;              ///< Enable per-primitive profiling.
    const bool use_out_of_order_queue;        ///< Enables parallel execution of primitives which don't depend on each other. Disabled by default.
    const std::string sources_dumps_dir;      ///< Specifies a directory where sources of cldnn::program objects should be dumped.
                                              ///< Empty by default (means no dumping).
    const priority_mode_types priority_mode;  ///< Priority mode (support of priority hints in command queue). If cl_khr_priority_hints extension
                                              ///< is not supported by current OpenCL implementation, the value must be set to cldnn_priority_disabled.

    const throttle_mode_types throttle_mode;  ///< Throttle mode (support of throttle hints in command queue). If cl_khr_throttle_hints extension
                                              ///< is not supported by current OpenCL implementation, the value must be set to cldnn_throttle_disabled.

    bool use_memory_pool;                     ///< Enables memory usage optimization. memory objects will be reused when possible
                                              ///< (switched off for older drivers then NEO).
    bool use_unified_shared_memory;           ///< Enables USM usage
    uint16_t n_streams;                       ///< Number of queues executed in parallel
    const std::string kernels_cache_path;     ///< Path to compiled kernels cache
    uint16_t n_threads;                       ///< Number of threads
    const std::string tuning_cache_path;      ///< Path to tuning kernel cache

    /// @brief Constructs engine configuration with specified options.
    /// @param enable_profiling Enable per-primitive profiling.
    /// @param dump_custom_program Dump the custom OpenCL programs to files
    /// @param single_kernel If provided, runs specific layer.
    engine_configuration(
        bool enable_profiling = false,
        bool use_out_of_order_queue = true,
        const std::string& sources_dumps_dir = std::string(),
        priority_mode_types priority_mode = priority_mode_types::disabled,
        throttle_mode_types throttle_mode = throttle_mode_types::disabled,
        bool use_memory_pool = true,
        bool use_unified_shared_memory = true,
        uint16_t n_streams = 1,
        const std::string& kernels_cache_path = "",
        uint16_t n_threads = std::max(static_cast<uint16_t>(std::thread::hardware_concurrency()), static_cast<uint16_t>(1)),
        const std::string& tuning_cache_path = "cache.json")
        : enable_profiling(enable_profiling)
        , use_out_of_order_queue(use_out_of_order_queue)
        , sources_dumps_dir(sources_dumps_dir)
        , priority_mode(priority_mode)
        , throttle_mode(throttle_mode)
        , use_memory_pool(use_memory_pool)
        , use_unified_shared_memory(use_unified_shared_memory)
        , n_streams(n_streams)
        , kernels_cache_path(kernels_cache_path)
        , n_threads(n_threads)
        , tuning_cache_path(tuning_cache_path) {
        if (n_streams == 0) {
            throw std::invalid_argument("Invalid streams count set in engine config");
        }
    }
};

/// @}

/// @}

}  // namespace cldnn
