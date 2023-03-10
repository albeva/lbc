//
// Created by Albert on 02/03/2022.
//
#pragma once
#include "pch.hpp"

namespace lbc {
class Token;

class TokenSource {
public:
    NO_COPY_AND_MOVE(TokenSource)
    TokenSource() = default;
    virtual ~TokenSource() = default;

    virtual unsigned int getFileId() = 0;
    virtual void next(Token& result) = 0;
    virtual void peek(Token& result) = 0;
};

} // namespace lbc
