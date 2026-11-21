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

#include "lib/rocprofiler-sdk/agent_mapping.hpp"

namespace rocprofiler
{
namespace agent
{
agent_mapping_result
compute_agent_mapping(const std::vector<int64_t>&                 logical_node_ids,
                      const std::vector<std::optional<uint32_t>>& driver_node_ids)
{
    auto result  = agent_mapping_result{};
    auto claimed = std::vector<bool>(logical_node_ids.size(), false);

    for(size_t hidx = 0; hidx < driver_node_ids.size(); ++hidx)
    {
        const auto& node_id = driver_node_ids.at(hidx);

        if(!node_id)
        {
            ++result.unqueryable_count;
            continue;
        }

        auto matched = false;
        for(size_t ridx = 0; ridx < logical_node_ids.size(); ++ridx)
        {
            if(claimed.at(ridx)) continue;
            if(logical_node_ids.at(ridx) != *node_id) continue;

            claimed.at(ridx) = true;
            result.pairs.emplace_back(
                agent_mapping_result::pair{ridx, hidx, *node_id});
            matched = true;
            break;
        }

        if(!matched) result.unmatched_node_ids.emplace_back(*node_id);
    }

    return result;
}
}  // namespace agent
}  // namespace rocprofiler
