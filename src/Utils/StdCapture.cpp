//
// Created by Albert on 02/04/2024.
//
#include "StdCapture.hpp"
#ifdef _MSC_VER
#    include <io.h>
#    define popen _popen
#    define pclose _pclose
#    define stat _stat
#    define dup _dup
#    define dup2 _dup2
#    define fileno _fileno
#    define close _close
#    define pipe _pipe
#    define read _read
#    define eof _eof
#else
#    include <unistd.h>
#endif
#include <cstdio>
#include <fcntl.h>

using namespace lbc;

CaptureStd CaptureStd::out() noexcept {
    return CaptureStd{ stdout };
}

CaptureStd CaptureStd::err() noexcept {
    return CaptureStd{ stderr };
}

CaptureStd::CaptureStd(FILE* stream) noexcept : m_stream(stream), m_fd(fileno(stream)) {
    // Make output stream unbuffered, so that we don't need to flush
    // the streams before capture and after capture (fflush can cause a deadlock)
    setvbuf(m_stream, nullptr, _IONBF, 0);

    // Start capturing.
    pipe(m_pipes);
    m_streamOld = dup(m_fd);
    dup2(m_pipes[WRITE], m_fd);
#ifndef _MSC_VER
    close(m_pipes[WRITE]);
#endif
}

std::string CaptureStd::finish() noexcept {
    // End capturing.
    dup2(m_streamOld, m_fd);

    constexpr size_t SIZE = 1025;
    char buf[SIZE]{};
    ssize_t bytesRead = 0;
    std::string output;
    output.reserve(SIZE);

    do {
#ifdef _MSC_VER
        if (!eof(pipes[READ])) {
            bytesRead = read(pipes[READ], buf, SIZE - 1);
        } else {
            break;
        }
#else
        bytesRead = read(m_pipes[READ], buf, SIZE - 1);
#endif
        if (bytesRead > 0) {
            buf[bytesRead] = 0;
            output.append(buf, static_cast<size_t>(bytesRead));
        }
    } while (bytesRead > 0);

    close(m_streamOld);
    close(m_pipes[READ]);
#ifdef _MSC_VER
    secure_close(pipes[WRITE]);
#endif
    return output;
}
