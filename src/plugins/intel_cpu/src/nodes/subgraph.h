// Copyright (C) 2020-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <oneapi/dnnl/dnnl_common.hpp>
#include <set>
#include <utility>
#include <vector>

#include "cpu_memory.h"
#include "cpu_types.h"
#include "emitters/snippets/input_repacker.hpp"
#include "executors/subgraph.hpp"
#include "graph_context.h"
#include "node.h"
#include "openvino/core/node.hpp"
#include "openvino/core/type/element_type.hpp"
#include "shape_inference/shape_inference_cpu.hpp"
#include "snippets/lowered/pass/pass.hpp"
#include "snippets/op/subgraph.hpp"
#include "snippets/pass/manager.hpp"

#if defined(OPENVINO_ARCH_ARM64)
#    include "cpu/aarch64/cpu_isa_traits.hpp"
#else
#    include "cpu/x64/cpu_isa_traits.hpp"
#endif

namespace ov::intel_cpu::node {

class Subgraph : public Node {
public:
    Subgraph(const std::shared_ptr<ov::Node>& op, const GraphContext::CPtr& context);
    ~Subgraph() override = default;

    void getSupportedDescriptors() override {};
    void initSupportedPrimitiveDescriptors() override;
    void selectOptimalPrimitiveDescriptor() override;
    ov::element::Type getRuntimePrecision() const override;

    void createPrimitive() override;
    void prepareParams() override;

    bool canBeInPlace() const override;
    bool created() const override;

    // if generator is set, it would execute generated code otherwise it would fallback to nGraph reference
    void execute(const dnnl::stream& strm) override;
    void executeDynamicImpl(const dnnl::stream& strm) override;

protected:
    IShapeInfer::Result shapeInfer() const override;

private:
    void initMemoryPtrs();
    void initAttributes();
    void initStartOffsets();
    void initPluginBlockedShapes() const;
    void optimizeIR();

    snippets::op::Subgraph::BlockedShapeVector getSnippetsBlockedShapes() const;
    std::pair<std::vector<ov::element::Type>, std::vector<ov::element::Type>> getIOPrecisions() const;

    static uint64_t getBodyHash(const std::shared_ptr<snippets::op::Subgraph>& snippet);
    uint32_t getBroadcastingMask(const std::vector<VectorDims>& input_shapes);
    std::set<size_t> getConstantInputIndexes() const;

    using DataFlowPasses = std::vector<ov::snippets::pass::Manager::PositionedPassBase>;
    using ControlFlowPasses = std::vector<ov::snippets::lowered::pass::PassPipeline::PositionedPassLowered>;

    DataFlowPasses getDataFlowPasses();
    ControlFlowPasses getControlFlowPasses();

    // Holds ISA version used is codeGeneration target
#if defined(OPENVINO_ARCH_ARM64)
#    define _ov_dnnl_cpu_isa dnnl::impl::cpu::aarch64::cpu_isa_t
#else
#    define _ov_dnnl_cpu_isa dnnl::impl::cpu::x64::cpu_isa_t
#endif

    _ov_dnnl_cpu_isa host_isa;

    std::shared_ptr<SubgraphAttrs> subgraph_attrs;

    // Index of Paramater -> Index of broadcastable dimension from end
    std::map<size_t, size_t> broadcastable_inputs;

    size_t input_num = 0;
    size_t output_num = 0;

    std::vector<MemoryPtr> srcMemPtrs;
    std::vector<MemoryPtr> dstMemPtrs;

    std::vector<ptrdiff_t> start_offset_in;
    std::vector<ptrdiff_t> start_offset_out;

    // Ptr indices which are not processed by the snippets kernel
    // and used directly in the specific emitters (e.g. jit_brgemm_emitter)
    std::set<size_t> external_ptrs_idces;

    bool is_dynamic = false;
    // Input shapes that are used in PrepareParams and ShapeInfer to avoid frequent memory allocation
    mutable std::vector<VectorDims> in_shapes;

    std::shared_ptr<SubgraphBaseExecutor> execPtr = nullptr;
};

}  // namespace ov::intel_cpu::node
