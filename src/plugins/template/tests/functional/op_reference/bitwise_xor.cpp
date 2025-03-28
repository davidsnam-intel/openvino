// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "openvino/op/bitwise_xor.hpp"

#include <gtest/gtest.h>

#include "bitwise.hpp"

using namespace ov;

namespace reference_tests {
namespace BitwiseOpsRefTestDefinitions {
namespace {

std::vector<RefBitwiseParams> generateBitwiseParams() {
    std::vector<RefBitwiseParams> bitwiseParams{
        Builder{}
            .opType(BitwiseTypes::BITWISE_XOR)
            .inputs({{{2, 2}, element::boolean, std::vector<char>{true, false, true, false}},
                     {{2, 2}, element::boolean, std::vector<char>{true, false, false, true}}})
            .expected({{2, 2}, element::boolean, std::vector<char>{false, false, true, true}}),
        Builder{}
            .opType(BitwiseTypes::BITWISE_XOR)
            .inputs(
                {{{3, 5},
                  element::u8,
                  std::vector<
                      uint8_t>{0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x1, 0x8, 0x8, 0xbf}},
                 {{3, 5},
                  element::u8,
                  std::vector<
                      uint8_t>{0x0, 0x1, 0x8, 0xbf, 0x3f, 0x1, 0x8, 0xbf, 0x3f, 0x8, 0xbf, 0x3f, 0xbf, 0x3f, 0x3f}}})
            .expected(
                {{3, 5},
                 element::u8,
                 std::vector<
                     uint8_t>{0xff, 0xfe, 0xf7, 0x40, 0xc0, 0x1, 0x8, 0xbf, 0x3f, 0x9, 0xbe, 0x3e, 0xb7, 0x37, 0x80}}),
        Builder{}
            .opType(BitwiseTypes::BITWISE_XOR)
            .inputs({{{3, 5},
                      element::u16,
                      std::vector<uint16_t>{0xffff,
                                            0xffff,
                                            0xffff,
                                            0xffff,
                                            0xffff,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x1,
                                            0x1,
                                            0x1,
                                            0x8,
                                            0x8,
                                            0xbfff}},
                     {{3, 5},
                      element::u16,
                      std::vector<uint16_t>{0x0,
                                            0x1,
                                            0x8,
                                            0xbfff,
                                            0x3fff,
                                            0x1,
                                            0x8,
                                            0xbfff,
                                            0x3fff,
                                            0x8,
                                            0xbfff,
                                            0x3fff,
                                            0xbfff,
                                            0x3fff,
                                            0x3fff}}})
            .expected({{3, 5},
                       element::u16,
                       std::vector<uint16_t>{0xffff,
                                             0xfffe,
                                             0xfff7,
                                             0x4000,
                                             0xc000,
                                             0x1,
                                             0x8,
                                             0xbfff,
                                             0x3fff,
                                             0x9,
                                             0xbffe,
                                             0x3ffe,
                                             0xbff7,
                                             0x3ff7,
                                             0x8000}}),
        Builder{}
            .opType(BitwiseTypes::BITWISE_XOR)
            .inputs({{{3, 5},
                      element::u32,
                      std::vector<uint32_t>{0xffffffff,
                                            0xffffffff,
                                            0xffffffff,
                                            0xffffffff,
                                            0xffffffff,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x1,
                                            0x1,
                                            0x1,
                                            0x8,
                                            0x8,
                                            0xbfffffff}},
                     {{3, 5},
                      element::u32,
                      std::vector<uint32_t>{0x0,
                                            0x1,
                                            0x8,
                                            0xbfffffff,
                                            0x3fffffff,
                                            0x1,
                                            0x8,
                                            0xbfffffff,
                                            0x3fffffff,
                                            0x8,
                                            0xbfffffff,
                                            0x3fffffff,
                                            0xbfffffff,
                                            0x3fffffff,
                                            0x3fffffff}}})
            .expected({{3, 5},
                       element::u32,
                       std::vector<uint32_t>{0xffffffff,
                                             0xfffffffe,
                                             0xfffffff7,
                                             0x40000000,
                                             0xc0000000,
                                             0x1,
                                             0x8,
                                             0xbfffffff,
                                             0x3fffffff,
                                             0x9,
                                             0xbffffffe,
                                             0x3ffffffe,
                                             0xbffffff7,
                                             0x3ffffff7,
                                             0x80000000}}),
        Builder{}
            .opType(BitwiseTypes::BITWISE_XOR)
            .inputs({{{3, 5},
                      element::u64,
                      std::vector<uint64_t>{0x0,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x1,
                                            0x1,
                                            0x1,
                                            0x1,
                                            0x4000000000000000,
                                            0x4000000000000000,
                                            0x4000000000000000,
                                            0xc000000000000000,
                                            0xc000000000000000,
                                            0xffffffffffffffff}},
                     {{3, 5},
                      element::u64,
                      std::vector<uint64_t>{0x1,
                                            0x4000000000000000,
                                            0xc000000000000000,
                                            0xffffffffffffffff,
                                            0x8,
                                            0x4000000000000000,
                                            0xc000000000000000,
                                            0xffffffffffffffff,
                                            0x8,
                                            0xc000000000000000,
                                            0xffffffffffffffff,
                                            0x8,
                                            0xffffffffffffffff,
                                            0x8,
                                            0x8}}})
            .expected({{3, 5},
                       element::u64,
                       std::vector<uint64_t>{0x1,
                                             0x4000000000000000,
                                             0xc000000000000000,
                                             0xffffffffffffffff,
                                             0x8,
                                             0x4000000000000001,
                                             0xc000000000000001,
                                             0xfffffffffffffffe,
                                             0x9,
                                             0x8000000000000000,
                                             0xbfffffffffffffff,
                                             0x4000000000000008,
                                             0x3fffffffffffffff,
                                             0xc000000000000008,
                                             0xfffffffffffffff7}}),
        Builder{}
            .opType(BitwiseTypes::BITWISE_XOR)
            .inputs(
                {{{3, 5},
                  element::i8,
                  std::vector<
                      uint8_t>{0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x1, 0x8, 0x8, 0xbf}},
                 {{3, 5},
                  element::i8,
                  std::vector<
                      uint8_t>{0x0, 0x1, 0x8, 0xbf, 0x3f, 0x1, 0x8, 0xbf, 0x3f, 0x8, 0xbf, 0x3f, 0xbf, 0x3f, 0x3f}}})
            .expected(
                {{3, 5},
                 element::i8,
                 std::vector<
                     uint8_t>{0xff, 0xfe, 0xf7, 0x40, 0xc0, 0x1, 0x8, 0xbf, 0x3f, 0x9, 0xbe, 0x3e, 0xb7, 0x37, 0x80}}),
        Builder{}
            .opType(BitwiseTypes::BITWISE_XOR)
            .inputs({{{3, 5},
                      element::i16,
                      std::vector<uint16_t>{0xffff,
                                            0xffff,
                                            0xffff,
                                            0xffff,
                                            0xffff,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x1,
                                            0x1,
                                            0x1,
                                            0x8,
                                            0x8,
                                            0xbfff}},
                     {{3, 5},
                      element::i16,
                      std::vector<uint16_t>{0x0,
                                            0x1,
                                            0x8,
                                            0xbfff,
                                            0x3fff,
                                            0x1,
                                            0x8,
                                            0xbfff,
                                            0x3fff,
                                            0x8,
                                            0xbfff,
                                            0x3fff,
                                            0xbfff,
                                            0x3fff,
                                            0x3fff}}})
            .expected({{3, 5},
                       element::i16,
                       std::vector<uint16_t>{0xffff,
                                             0xfffe,
                                             0xfff7,
                                             0x4000,
                                             0xc000,
                                             0x1,
                                             0x8,
                                             0xbfff,
                                             0x3fff,
                                             0x9,
                                             0xbffe,
                                             0x3ffe,
                                             0xbff7,
                                             0x3ff7,
                                             0x8000}}),
        Builder{}
            .opType(BitwiseTypes::BITWISE_XOR)
            .inputs({{{3, 5},
                      element::i32,
                      std::vector<uint32_t>{0xffffffff,
                                            0xffffffff,
                                            0xffffffff,
                                            0xffffffff,
                                            0xffffffff,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x1,
                                            0x1,
                                            0x1,
                                            0x8,
                                            0x8,
                                            0xbfffffff}},
                     {{3, 5},
                      element::i32,
                      std::vector<uint32_t>{0x0,
                                            0x1,
                                            0x8,
                                            0xbfffffff,
                                            0x3fffffff,
                                            0x1,
                                            0x8,
                                            0xbfffffff,
                                            0x3fffffff,
                                            0x8,
                                            0xbfffffff,
                                            0x3fffffff,
                                            0xbfffffff,
                                            0x3fffffff,
                                            0x3fffffff}}})
            .expected({{3, 5},
                       element::i32,
                       std::vector<uint32_t>{0xffffffff,
                                             0xfffffffe,
                                             0xfffffff7,
                                             0x40000000,
                                             0xc0000000,
                                             0x1,
                                             0x8,
                                             0xbfffffff,
                                             0x3fffffff,
                                             0x9,
                                             0xbffffffe,
                                             0x3ffffffe,
                                             0xbffffff7,
                                             0x3ffffff7,
                                             0x80000000}}),
        Builder{}
            .opType(BitwiseTypes::BITWISE_XOR)
            .inputs({{{3, 5},
                      element::i64,
                      std::vector<uint64_t>{0x0,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x0,
                                            0x1,
                                            0x1,
                                            0x1,
                                            0x1,
                                            0x4000000000000000,
                                            0x4000000000000000,
                                            0x4000000000000000,
                                            0xc000000000000000,
                                            0xc000000000000000,
                                            0xffffffffffffffff}},
                     {{3, 5},
                      element::i64,
                      std::vector<uint64_t>{0x1,
                                            0x4000000000000000,
                                            0xc000000000000000,
                                            0xffffffffffffffff,
                                            0x8,
                                            0x4000000000000000,
                                            0xc000000000000000,
                                            0xffffffffffffffff,
                                            0x8,
                                            0xc000000000000000,
                                            0xffffffffffffffff,
                                            0x8,
                                            0xffffffffffffffff,
                                            0x8,
                                            0x8}}})
            .expected({{3, 5},
                       element::i64,
                       std::vector<uint64_t>{0x1,
                                             0x4000000000000000,
                                             0xc000000000000000,
                                             0xffffffffffffffff,
                                             0x8,
                                             0x4000000000000001,
                                             0xc000000000000001,
                                             0xfffffffffffffffe,
                                             0x9,
                                             0x8000000000000000,
                                             0xbfffffffffffffff,
                                             0x4000000000000008,
                                             0x3fffffffffffffff,
                                             0xc000000000000008,
                                             0xfffffffffffffff7}}),
    };
    return bitwiseParams;
}

INSTANTIATE_TEST_SUITE_P(smoke_BitwiseAnd_With_Hardcoded_Refs,
                         ReferenceBitwiseLayerTest,
                         ::testing::ValuesIn(generateBitwiseParams()),
                         ReferenceBitwiseLayerTest::getTestCaseName);

}  // namespace
}  // namespace BitwiseOpsRefTestDefinitions
}  // namespace reference_tests
