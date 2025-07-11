// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "shared_test_classes/subgraph/lora_pattern.hpp"

#include "common_test_utils/node_builders/eltwise.hpp"
#include "common_test_utils/node_builders/convolution.hpp"
#include "common_test_utils/ov_tensor_utils.hpp"
#include "shared_test_classes/base/ov_subgraph.hpp"
#include "template/properties.hpp"
#include "openvino/op/add.hpp"
#include "openvino/op/convert.hpp"
#include "openvino/op/divide.hpp"
#include "openvino/op/gather.hpp"
#include "openvino/op/matmul.hpp"
#include "openvino/op/multiply.hpp"
#include "openvino/op/shape_of.hpp"
#include "openvino/op/transpose.hpp"

namespace ov {
namespace test {

void LoraPatternBase::run_test_empty_tensors() {
    compile_model();
    inferRequest = compiledModel.create_infer_request();
    ASSERT_TRUE(inferRequest);
    generate_inputs(targetStaticShapes.front());
    for (const auto& input : inputs) {
        inferRequest.set_tensor(input.first, input.second);
    }

    inferRequest.infer();
    auto outputs = function->outputs();

    auto tx_result = inferRequest.get_tensor(outputs[0]);
    auto tz_result = inferRequest.get_tensor(outputs[1]);
    ov::test::utils::compare(tx_result, tz_result);
}

void LoraPatternBase::run_test_random_tensors(ov::element::Type net_type, size_t lora_rank) {
    compile_model();
    inferRequest = compiledModel.create_infer_request();
    ASSERT_TRUE(inferRequest);

    // use the Template plugin as a reference

    auto compiledReferenceModel = core->compile_model(function,
                                                      ov::test::utils::DEVICE_TEMPLATE,
                                                      {{ov::template_plugin::disable_transformations(true)}});
    auto inferRequestRef = compiledReferenceModel.create_infer_request();
    ASSERT_TRUE(inferRequestRef);

    generate_inputs(targetStaticShapes.front());
    for (const auto& input : inputs) {
        inferRequest.set_tensor(input.first, input.second);
        inferRequestRef.set_tensor(input.first, input.second);
    }

    constexpr int infer_count = 6lu;

    std::unordered_map<std::string, ov::Shape> stateShapes;

    auto&& vars = function->get_variables();

    for (auto&& var : vars) {
        auto var_info = var->get_info();
        auto var_shape = var_info.data_shape;

        std::for_each(var_shape.begin(), var_shape.end(), [=](ov::PartialShape::value_type& x) {
            if (x.is_dynamic()) {
                x = lora_rank;
            }
        });
        stateShapes.insert({var_info.variable_id, var_shape.to_shape()});
    }

    for (int i = 0; i < infer_count; ++i) {
        // set states

        auto&& states = inferRequest.query_state();
        if (!(i & 0x1)) { // every even call
            // generate and set state tensors
            for (auto&& item : states) {
                auto&& refStates = inferRequestRef.query_state();
                using ov::test::utils::InputGenerateData;
                const auto& shape = stateShapes.at(item.get_name());
                ov::Tensor tensor;
                if (net_type == ov::element::f16) {
                    tensor = ov::test::utils::create_and_fill_tensor(net_type, shape, InputGenerateData{0, 1, 10, i});
                } else {
                    tensor = ov::test::utils::create_and_fill_tensor(net_type, shape, InputGenerateData{0, 10, 1, i});
                }
                item.set_state(tensor);
                auto itr = std::find_if(refStates.begin(), refStates.end(), [&](const ov::VariableState& state) {
                    return state.get_name() == item.get_name();
                });
                ASSERT_FALSE(itr == refStates.end());
                itr->set_state(tensor);
            }
        }

        inferRequest.infer();
        inferRequestRef.infer();
        auto outputs = function->outputs();

        auto tx_result = inferRequest.get_tensor(outputs[0]);
        auto tz_result = inferRequest.get_tensor(outputs[1]);

        auto tx_result_ref = inferRequestRef.get_tensor(outputs[0]);
        auto tz_result_ref = inferRequestRef.get_tensor(outputs[1]);

        float abs_threshold = 1e-4f;
        float rel_threshold = 1e-4f;
        if (net_type == ov::element::f16) {
            abs_threshold = 1e-2f;
            rel_threshold = 4e-2f;
        }
        ov::test::utils::compare(tx_result, tx_result_ref, abs_threshold, rel_threshold);
        ov::test::utils::compare(tz_result, tz_result_ref, abs_threshold, rel_threshold);
    }
}

std::string LoraPatternMatmul::getTestCaseName(testing::TestParamInfo<LoraMatMulParams> obj) {
    std::string device_name;
    ov::element::Type net_type;
    size_t M, N, K, lora_rank;
    std::tie(device_name, net_type, M, N, K, lora_rank) = obj.param;

    std::ostringstream result;
    result << "M=" << M << "_";
    result << "N=" << N << "_";
    result << "K=" << K << "_";
    result << "LoraRank=" << lora_rank << "_";
    result << "netType=" << net_type << "_";
    result << "targetDevice=" << device_name;

    return result.str();
}

void LoraPatternMatmul::SetUp() {
    ov::element::Type netType;
    size_t M, N, K, lora_rank;

    std::tie(targetDevice, netType, M, N, K, lora_rank) = this->GetParam();

    ov::PartialShape shape_x = {-1, -1, K};
    ov::PartialShape shape_w = {N, K};

    auto param_y = std::make_shared<ov::op::v0::Parameter>(netType, shape_x);
    auto param_w = std::make_shared<ov::op::v0::Parameter>(netType, shape_w);

    // "Main" matrix multiplication from the original transformer model
    auto tx = std::make_shared<ov::op::v0::MatMul>(param_y, param_w, false, true);

    // LoRA parameters from states
    auto variable_t4 = std::make_shared<ov::op::util::Variable>(
        ov::op::util::VariableInfo{ov::PartialShape({N, -1}), netType, t4_name});
    auto t4 = std::make_shared<ov::op::v6::ReadValue>(variable_t4);
    auto t4_assign = std::make_shared<ov::op::v6::Assign>(t4, variable_t4);

    auto variable_t5 = std::make_shared<ov::op::util::Variable>(
        ov::op::util::VariableInfo{ov::PartialShape({1, -1}), netType, t5_name});
    auto t5 = std::make_shared<ov::op::v6::ReadValue>(variable_t5);
    auto t5_assign = std::make_shared<ov::op::v6::Assign>(t5, variable_t5);

    auto variable_t6 = std::make_shared<ov::op::util::Variable>(
        ov::op::util::VariableInfo{ov::PartialShape({-1, K}), netType, t6_name});
    auto t6 = std::make_shared<ov::op::v6::ReadValue>(variable_t6);
    auto t6_assign = std::make_shared<ov::op::v6::Assign>(t6, variable_t6);

    auto shape_of_state_a = std::make_shared<ov::op::v3::ShapeOf>(t6);
    auto indices_node = ov::op::v0::Constant::create(ov::element::i32, ov::Shape(), {0});
    auto axis_node = ov::op::v0::Constant::create(ov::element::i32, ov::Shape(), {0});
    auto gather = std::make_shared<ov::op::v8::Gather>(shape_of_state_a, indices_node, axis_node);
    auto convert = std::make_shared<ov::op::v0::Convert>(gather, netType);
    auto divide = std::make_shared<ov::op::v1::Divide>(t5, convert);

    // Apply LoRA parameters to the current activations
    auto t5810 = std::make_shared<ov::op::v0::MatMul>(param_y, t6, false, true);
    auto t5811 = std::make_shared<ov::op::v1::Multiply>(t5810, divide);
    auto t5812 = std::make_shared<ov::op::v0::MatMul>(t5811, t4, false, true);

    // Mix LoRA part into normally computed activations after the "main" MatMul
    auto tz = std::make_shared<ov::op::v1::Add>(tx, t5812);

    auto result_x = std::make_shared<ov::op::v0::Result>(tx);
    auto result_z = std::make_shared<ov::op::v0::Result>(tz);

    function = std::make_shared<ov::Model>(ov::ResultVector({result_x, result_z}),
                                           ov::SinkVector({t4_assign, t5_assign, t6_assign}),
                                           ov::ParameterVector({param_y, param_w}));
}

std::string LoraPatternConvolution::getTestCaseName(testing::TestParamInfo<LoraConvolutionParams> obj) {
    std::string device_name;
    ov::element::Type net_type;
    size_t num_channels, lora_rank;
    std::tie(device_name, net_type, num_channels, lora_rank) = obj.param;

    std::ostringstream result;
    result << "NumChannels=" << num_channels << "_";
    result << "LoraRank=" << lora_rank << "_";
    result << "netType=" << net_type << "_";
    result << "targetDevice=" << device_name;

    return result.str();
}

void LoraPatternConvolution::SetUp() {
    ov::element::Type netType;
    size_t num_channels, lora_rank;

    std::tie(targetDevice, netType, num_channels, lora_rank) = this->GetParam();

    ov::PartialShape shape_x = {-1, num_channels, -1, -1};

    auto param_y = std::make_shared<ov::op::v0::Parameter>(netType, shape_x);

    // Original Convolution that is modified by LoRA adapter later
    auto tx = ov::test::utils::make_convolution(param_y,
                                                netType,
                                                {1, 1},
                                                {1, 1},
                                                {0, 0},
                                                {0, 0},
                                                {1, 1},
                                                ov::op::PadType::EXPLICIT,
                                                num_channels);

    // LoRA parameters from states
    auto variable_t4 = std::make_shared<ov::op::util::Variable>(
        ov::op::util::VariableInfo{ov::PartialShape({num_channels, -1}), netType, t4_name});
    auto t4 = std::make_shared<ov::op::v6::ReadValue>(variable_t4);
    auto t4_assign = std::make_shared<ov::op::v6::Assign>(t4, variable_t4);

    auto variable_t5 = std::make_shared<ov::op::util::Variable>(
        ov::op::util::VariableInfo{ov::PartialShape({1, -1}), netType, t5_name});
    auto t5 = std::make_shared<ov::op::v6::ReadValue>(variable_t5);
    auto t5_assign = std::make_shared<ov::op::v6::Assign>(t5, variable_t5);

    auto variable_t6 = std::make_shared<ov::op::util::Variable>(
        ov::op::util::VariableInfo{ov::PartialShape({-1, num_channels}), netType, t6_name});
    auto t6 = std::make_shared<ov::op::v6::ReadValue>(variable_t6);
    auto t6_assign = std::make_shared<ov::op::v6::Assign>(t6, variable_t6);

    auto shape_of_state_a = std::make_shared<ov::op::v3::ShapeOf>(t6);
    auto indices_node = ov::op::v0::Constant::create(ov::element::i32, ov::Shape(), {0});
    auto axis_node = ov::op::v0::Constant::create(ov::element::i32, ov::Shape(), {0});
    auto gather = std::make_shared<ov::op::v8::Gather>(shape_of_state_a, indices_node, axis_node);
    auto convert = std::make_shared<ov::op::v0::Convert>(gather, netType);
    auto divide = std::make_shared<ov::op::v1::Divide>(t5, convert);

    // LoRA pattern with additional Transposes to move channel dimensions into positions where MatMul can be applied
    auto t4940 =
        std::make_shared<ov::op::v0::Constant>(ov::element::i32, ov::Shape{4}, std::vector<size_t>{2, 3, 0, 1});

    auto t4941 = std::make_shared<ov::op::v1::Transpose>(param_y, t4940);
    auto t4942 = std::make_shared<ov::op::v0::MatMul>(t4941, t6, false, true);
    auto t4943 = std::make_shared<ov::op::v1::Multiply>(t4942, divide);
    auto t4944 = std::make_shared<ov::op::v0::MatMul>(t4943, t4, false, true);

    auto t4945 =
        std::make_shared<ov::op::v0::Constant>(ov::element::i32, ov::Shape{4}, std::vector<size_t>{2, 3, 0, 1});
    auto t4946 = std::make_shared<ov::op::v1::Transpose>(t4944, t4945);

    // Mix LoRA part into normally computed activations after the "main" MatMul
    auto tz = std::make_shared<ov::op::v1::Add>(tx, t4946);

    auto result_x = std::make_shared<ov::op::v0::Result>(tx);
    auto result_z = std::make_shared<ov::op::v0::Result>(tz);

    function = std::make_shared<ov::Model>(ov::ResultVector({result_x, result_z}),
                                           ov::SinkVector({t4_assign, t5_assign, t6_assign}),
                                           ov::ParameterVector({param_y}));
}

}  // namespace test
}  // namespace ov
