#include "Sample.hpp"
#include <cmake/config.hpp>
using namespace lbc::utils;

auto Sample::getMessage() noexcept -> std::expected<std::string_view, bool> {
    return "Hello World!"sv;
}
