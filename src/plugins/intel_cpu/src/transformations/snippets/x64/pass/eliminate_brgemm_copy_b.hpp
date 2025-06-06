// Copyright (C) 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>
#include <memory>
#include <set>
#include <utility>

#include "emitters/snippets/repacked_input.hpp"
#include "openvino/core/model.hpp"
#include "openvino/pass/pass.hpp"

namespace ov::intel_cpu::pass {

/**
 * @interface EliminateBrgemmCopyB
 * @brief EliminateBrgemmCopyB identifies BrgemmCopyB nodes which can be inferred outside the Subgraph.
 * If this is possible, CopyB node is removed, and the external repacking is configured on the further pipeline stages
 * in RuntimeConfigurator.
 *
 * @ingroup snippets
 */
class EliminateBrgemmCopyB : public ov::pass::ModelPass {
public:
    OPENVINO_MODEL_PASS_RTTI("EliminateBrgemmCopyB");
    EliminateBrgemmCopyB(ov::intel_cpu::RepackedInputConfig& repacked_runtime_inputs_config,
                         ov::intel_cpu::RepackedInputConfig& repacked_constant_inputs_config)
        : m_repacked_runtime_inputs_config(repacked_runtime_inputs_config),
          m_repacked_constant_inputs_config(repacked_constant_inputs_config) {}

    bool run_on_model(const std::shared_ptr<ov::Model>& model) override;

private:
    ov::intel_cpu::RepackedInputConfig& m_repacked_runtime_inputs_config;
    ov::intel_cpu::RepackedInputConfig& m_repacked_constant_inputs_config;
};

}  // namespace ov::intel_cpu::pass
