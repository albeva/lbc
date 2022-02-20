#include <utility>

//
// Created by Albert on 19/02/2022.
//
#pragma once

namespace lbc {
enum class Diag;

class ParseError final : public llvm::ErrorInfo<ParseError> {
public:
    static const char ID;

    explicit ParseError(Diag diag, std::string message, llvm::SMRange range) noexcept
    : m_diag{ diag }, m_message{std::move(message)}, m_range{ range } {}

    [[nodiscard]] Diag getDiag() const noexcept { return m_diag; }
    [[nodiscard]] const string& getMessage() const noexcept { return m_message; }
    [[nodiscard]] const llvm::SMRange& getRange() const noexcept { return m_range; }

    void log(llvm::raw_ostream& os) const override {
        os << m_message;
    }

    [[nodiscard]] std::error_code convertToErrorCode() const override {
        return llvm::inconvertibleErrorCode();
    }

private:
    Diag m_diag;
    string m_message;
    llvm::SMRange m_range;
};

} // namespace lbc
