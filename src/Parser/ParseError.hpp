//
// Created by Albert on 19/02/2022.
//
#pragma once
namespace lbc {

class ParseError: public llvm::ErrorInfo<ParseError> {
public:
    static char ID;

    void log(llvm::raw_ostream &os) const override {
        os << "parse error";
    }

    std::error_code convertToErrorCode() const override {
        return llvm::inconvertibleErrorCode();
    }
};
}
