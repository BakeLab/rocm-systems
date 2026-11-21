// MIT License
//
// Copyright (c) 2023-2026 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Pairing of rocprofiler agent records with HSA agents. Pure integer matching:
// no HSA runtime or GPU is involved, so this runs anywhere.

#include "lib/rocprofiler-sdk/agent_mapping.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

using rocprofiler::agent::compute_agent_mapping;

// On bare metal rocprofiler and HSA both walk the KFD sysfs tree, so the dense
// logical ordinal and the driver node id agree and every agent pairs up.
TEST(agent_mapping, bare_metal_maps_every_agent)
{
    const auto rocp = std::vector<int64_t>{0, 1, 2};
    const auto hsa  = std::vector<std::optional<uint32_t>>{0, 1, 2};

    const auto mapping = compute_agent_mapping(rocp, hsa);

    ASSERT_EQ(mapping.pairs.size(), 3);
    EXPECT_TRUE(mapping.complete());
    for(const auto& itr : mapping.pairs)
    {
        EXPECT_EQ(itr.rocp_index, itr.hsa_index);
        EXPECT_EQ(itr.key, hsa.at(itr.hsa_index).value());
    }
}

// An HSA GPU with no rocprofiler counterpart is an internal inconsistency on
// bare metal; the caller turns an incomplete mapping into a fatal.
TEST(agent_mapping, bare_metal_reports_an_unmatched_hsa_gpu)
{
    const auto rocp = std::vector<int64_t>{0, 1};
    const auto hsa  = std::vector<std::optional<uint32_t>>{0, 1, 2};

    const auto mapping = compute_agent_mapping(rocp, hsa);

    EXPECT_EQ(mapping.pairs.size(), 2);
    EXPECT_FALSE(mapping.complete());
    ASSERT_EQ(mapping.unmatched_node_ids.size(), 1);
    EXPECT_EQ(mapping.unmatched_node_ids.front(), 2);
}

// --- shared invariants -----------------------------------------------------

// Two HSA agents reporting the same driver node id must not both alias onto
// the one rocprofiler record: the second stays unmatched.
TEST(agent_mapping, a_rocprofiler_agent_is_claimed_at_most_once)
{
    const auto rocp = std::vector<int64_t>{1};
    const auto hsa  = std::vector<std::optional<uint32_t>>{1, 1};

    const auto mapping = compute_agent_mapping(rocp, hsa);
    ASSERT_EQ(mapping.pairs.size(), 1);
    EXPECT_EQ(mapping.pairs.front().hsa_index, 0);
    EXPECT_EQ(mapping.unmatched_node_ids.size(), 1);
    EXPECT_FALSE(mapping.complete());
}

// An HSA agent whose driver node id could not be read is unpairable under
// every policy and is reported separately from a genuine mismatch.
TEST(agent_mapping, an_unqueryable_hsa_agent_is_counted_not_matched)
{
    const auto rocp = std::vector<int64_t>{0};
    const auto hsa  = std::vector<std::optional<uint32_t>>{std::nullopt};

    const auto mapping = compute_agent_mapping(rocp, hsa);
    EXPECT_TRUE(mapping.pairs.empty());
    EXPECT_EQ(mapping.unqueryable_count, 1);
    EXPECT_TRUE(mapping.unmatched_node_ids.empty());
    EXPECT_FALSE(mapping.complete());
}

// A rocprofiler agent HSA does not report is not an error in either direction:
// nothing is unmatched because the mapping is driven by the HSA list.
TEST(agent_mapping, a_surplus_rocprofiler_agent_is_not_a_mismatch)
{
    const auto rocp = std::vector<int64_t>{0, 1};
    const auto hsa  = std::vector<std::optional<uint32_t>>{0};

    const auto mapping = compute_agent_mapping(rocp, hsa);
    EXPECT_EQ(mapping.pairs.size(), 1);
    EXPECT_TRUE(mapping.complete());
}

TEST(agent_mapping, empty_inputs_are_a_complete_mapping)
{
    const auto mapping = compute_agent_mapping({}, {});
    EXPECT_TRUE(mapping.pairs.empty());
    EXPECT_TRUE(mapping.complete());
}
