// Compat shim: the recomp uses the {fmt} library (fetched via CMake). The lib only
// needs fmt::format, which is API-compatible with C++20 std::format — so map it. A
// host that already vendors real {fmt} can drop this compat dir from its include path.
#pragma once

#include <format>
#include <string>
#include <utility>

namespace fmt
{
    template <typename... Args>
    inline std::string format(std::format_string<Args...> fmtStr, Args&&... args)
    {
        return std::format(fmtStr, std::forward<Args>(args)...);
    }
}
