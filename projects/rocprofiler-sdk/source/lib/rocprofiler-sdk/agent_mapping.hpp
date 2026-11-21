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

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace rocprofiler
{
namespace agent
{
// Pairing of rocprofiler agent records with HSA agents.
//
// Split out of construct_agent_cache() so the decision can be exercised without
// an HSA runtime or GPU: everything below is a pure function of
// two lists of integers.

struct agent_mapping_result
{
    struct pair
    {
        size_t rocp_index = 0;
        size_t hsa_index  = 0;
        // Key construct_agent_cache() files this pair under. Equal to the
        // driver node id, which under the strict policy is also the matched
        // agent's logical_node_id.
        uint32_t key = 0;
    };

    std::vector<pair> pairs;

    std::vector<uint32_t> unmatched_node_ids;

    size_t unqueryable_count = 0;

    bool complete() const { return unmatched_node_ids.empty() && unqueryable_count == 0; }
};

// Pair every HSA agent with at most one rocprofiler agent. Each rocprofiler
// agent is claimed once, so two HSA agents reporting the same driver node id
// leave the second unmatched rather than both aliasing onto one record.
agent_mapping_result
compute_agent_mapping(const std::vector<int64_t>&                 logical_node_ids,
                      const std::vector<std::optional<uint32_t>>& driver_node_ids);
}  // namespace agent
}  // namespace rocprofiler
