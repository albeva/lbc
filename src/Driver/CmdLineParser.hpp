//
// Created by Albert Varaksin on 17/04/2021.
//
#pragma once
#include "pch.hpp"

namespace lbc {
class CompileOptions;

/**
 * Parse given commandline arguments
 * and build up the context
 */
class CmdLineParser final {
public:
    NO_COPY_AND_MOVE(CmdLineParser)

    using Args = llvm::ArrayRef<const char*>;

    explicit CmdLineParser(CompileOptions& options) : m_options{ options } {}
    ~CmdLineParser() = default;

    void parse(const Args& args);

private:
    void processOption(const Args& args, size_t& index);

    [[noreturn]] static void showError(const std::string& message);
    [[noreturn]] static void showHelp();
    [[noreturn]] static void showVersion();

    CompileOptions& m_options;
};

} // namespace lbc
