#pragma once
#include "pch.hpp"

namespace lbc::utils {
class Sample final {
public:
    [[nodiscard]] static auto getMessage() noexcept -> std::expected<std::string_view, bool>;
};
} // namespace lbc::utils
