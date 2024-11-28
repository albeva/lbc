//
// Created by Albert Varaksin on 05/07/2020.
//
#include "Ast.hpp"
#include "Type/Type.hpp"
#include <algorithm>
using namespace lbc;

namespace {
// clang-format off
constexpr std::array nodes {
    #define KIND_ENUM(id, ...) llvm::StringLiteral { "Ast" #id },
    AST_CONTENT_NODES(KIND_ENUM)
    #undef KIND_ENUM
};
// clang-format on
} // namespace

auto AstRoot::getClassName() const -> llvm::StringRef {
    auto index = static_cast<size_t>(kind);
    return nodes.at(index);
}

auto AstAttributeList::getStringLiteral(llvm::StringRef key) const -> std::optional<llvm::StringRef> {
    for (const auto& attr : attribs) {
        if (attr->identExpr->name != key) {
            continue;
        }

        if (attr->args->exprs.size() != 1) {
            fatalError("Attribute "_t + key + " must have 1 value", false);
        }

        if (auto* literal = llvm::dyn_cast<AstLiteralExpr>(attr->args->exprs[0])) {
            if (const auto* str = std::get_if<llvm::StringRef>(&literal->getValue())) {
                return *str;
            }
            fatalError("Attribute "_t + key + " must be a string literal", false);
        }
    }
    return std::nullopt;
}

auto AstAttributeList::exists(llvm::StringRef name) const -> bool {
    return std::ranges::any_of(attribs, [&](const auto& attr) {
        return attr->identExpr->name == name;
    });
}
