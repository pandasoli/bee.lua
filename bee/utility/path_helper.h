#pragma once

#include <bee/nonstd/expected.h>
#include <bee/nonstd/filesystem.h>

namespace bee::path_helper {
    using path_expected = expected<fs::path, std::string>;
    path_expected exe_path() noexcept;
    path_expected dll_path() noexcept;
    bool equal(const fs::path& lhs, const fs::path& rhs) noexcept;
}
