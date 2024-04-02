//
// Created by Albert on 02/04/2024.
//
#pragma once
#include "pch.hpp"

namespace lbc {

class CaptureStd final {
public:
    /// Capture stdout
    static CaptureStd out() noexcept;

    /// Capture stderr
    static CaptureStd err() noexcept;

    /// Capture given stream
    explicit CaptureStd(FILE* stream) noexcept;

    /// Finish capturing and return the result as a string
    std::string finish() noexcept;

private:
    FILE* m_stream;
    int m_fd;

    enum PIPES {
        READ,
        WRITE
    };

    int m_pipes[2]{};
    int m_streamOld;
};

} // namespace lbc
