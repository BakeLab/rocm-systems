// MIT License
//
// Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved.
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

#include "lib/rocprofiler-sdk/agent.hpp"
#include "lib/rocprofiler-sdk/platform/gnulinux/agent.hpp"

#include "lib/common/environment.hpp"
#include "lib/common/filesystem.hpp"
#include "lib/common/utility.hpp"
#include "lib/rocprofiler-sdk/registration.hpp"

#include <gtest/gtest.h>

#include <unistd.h>

#include <cstdlib>
#include <optional>

namespace fs       = ::rocprofiler::common::filesystem;
namespace common   = ::rocprofiler::common;
namespace agent    = ::rocprofiler::agent;
namespace platform = ::rocprofiler::platform;

namespace
{
// Save/restore the env vars this test mutates so a failure in one TEST does
// not leak state into the next one.
class EnvGuard
{
public:
    explicit EnvGuard(std::initializer_list<std::string_view> names)
    {
        for(auto n : names)
        {
            const auto* value = ::getenv(n.data());
            saved_.emplace_back(n, value ? std::make_optional<std::string>(value) : std::nullopt);
        }
    }
    ~EnvGuard()
    {
        for(const auto& [n, v] : saved_)
        {
            if(v)
                common::set_env(n, *v, 1);
            else
                ::unsetenv(n.c_str());
        }
    }

    EnvGuard(const EnvGuard&) = delete;
    EnvGuard& operator=(const EnvGuard&) = delete;

private:
    std::vector<std::pair<std::string, std::optional<std::string>>> saved_;
};

fs::path
local_topology_path()
{
    auto cmdline = common::read_command_line(getpid());
    if(cmdline.empty()) return {};
    auto exe = fs::path{cmdline.at(0)};
    if(!fs::exists(exe)) return {};
    auto dir = fs::canonical(exe).parent_path() / "data" / "topology" / "nodes";
    return fs::exists(dir) ? dir : fs::path{};
}
}  // namespace

// gnulinux::is_available() must accept the test fixture's sysfs tree via
// the documented env override. This is the same fixture used by
// agent_local_topology in agent.cpp.
TEST(platform_dispatch, gnulinux_is_available_with_local_topology)
{
    auto topo = local_topology_path();
    if(topo.empty()) GTEST_SKIP() << "test data 'data/topology/nodes' not found next to binary";

    EnvGuard guard{{"ROCPROFILER_KFD_TOPOLOGY", "AMD_KFD_TOPOLOGY", "HSA_MODEL_TOPOLOGY"}};
    common::set_env("ROCPROFILER_KFD_TOPOLOGY", topo.string(), 1);
    common::set_env("AMD_KFD_TOPOLOGY", std::string{}, 1);
    common::set_env("HSA_MODEL_TOPOLOGY", std::string{}, 1);

    EXPECT_TRUE(platform::gnulinux::is_available());
}

// Native enumeration also uses the configured topology during late attach.
TEST(platform_dispatch, late_attach_marker_does_not_change_gnulinux_enumeration)
{
    auto topo = local_topology_path();
    if(topo.empty()) GTEST_SKIP() << "test data 'data/topology/nodes' not found next to binary";

    EnvGuard guard{{"ROCPROFILER_REGISTER_TOOL_ATTACHED",
                    "ROCPROFILER_KFD_TOPOLOGY",
                    "AMD_KFD_TOPOLOGY",
                    "HSA_MODEL_TOPOLOGY"}};
    common::set_env("ROCPROFILER_REGISTER_TOOL_ATTACHED", std::string{"1"}, 1);
    common::set_env("ROCPROFILER_KFD_TOPOLOGY", topo.string(), 1);
    common::set_env("AMD_KFD_TOPOLOGY", std::string{}, 1);
    common::set_env("HSA_MODEL_TOPOLOGY", std::string{}, 1);

    rocprofiler::registration::init_logging();
    agent::internal_refresh_topology();
    EXPECT_FALSE(agent::get_agents().empty());
}
