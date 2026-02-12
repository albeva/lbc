#pragma once
#include "pch.hpp"

namespace lbc::utils {
    class Sample final {
        public:
        [[nodiscard]] static auto getMessage() noexcept -> std::string_view;
    };
}
