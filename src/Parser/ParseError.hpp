//
// Created by Albert on 19/02/2022.
//
#pragma once
namespace lbc {

enum class Diag;

class ParseError final : public llvm::ErrorInfo<ParseError> {
public:
    static char ID;

    explicit ParseError(Diag diag, const std::string& message, llvm::SMRange range) noexcept
    : m_diag{ diag }, m_message{ message }, m_range{ range } {}

    Diag getDiag() const noexcept { return m_diag; }
    const string& getMessage() const noexcept { return m_message; }
    const llvm::SMRange& getRange() const noexcept { return m_range; }

    void log(llvm::raw_ostream& os) const override {
        os << m_message;
    }

    std::error_code convertToErrorCode() const override {
        return llvm::inconvertibleErrorCode();
    }

private:
    Diag m_diag;
    string m_message;
    llvm::SMRange m_range;
};
} // namespace lbc
