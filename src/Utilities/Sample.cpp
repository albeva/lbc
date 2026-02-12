#include "Sample.hpp"
using namespace lbc::utils;

auto Sample::getMessage() noexcept-> std::string_view{
    return "Hello World!"sv;
}
