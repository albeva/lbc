//
// Created by Albert Varaksin on 17/04/2021.
//
#pragma once
#include "pch.hpp"

namespace lbc {

struct CompileOptions;

/**
 * @class CmdLineParser
 * @brief Parses given commandline arguments and builds up the context.
 *
 * This class is responsible for parsing the command line arguments
 * provided to the program and setting the appropriate options in
 * the CompileOptions object.
 */
class CmdLineParser final {
public:
    NO_COPY_AND_MOVE(CmdLineParser)

    using Args = llvm::ArrayRef<const char*>;

    /**
     * @brief Construct a new CmdLineParser object.
     * @param options Reference to a CompileOptions object where the parsed options will be stored.
     */
    explicit CmdLineParser(CompileOptions& options)
    : m_options { options } {
    }

    /**
     * @brief Destroy the CmdLineParser object.
     */
    ~CmdLineParser() = default;

    /**
     * @brief Parse the command line arguments.
     * @param args Array of command line arguments.
     */
    void parse(const Args& args);

private:
    /**
     * @brief Process a single command line option.
     * @param args Array of command line arguments.
     * @param index Index of the current argument in the array.
     */
    void processOption(const Args& args, size_t& index);

    /**
     * @brief Show an error message and terminate the program.
     * @param message Error message to be displayed.
     */
    [[noreturn]] static void showError(const std::string& message);

    /**
     * @brief Show the help message and terminate the program.
     */
    [[noreturn]] static void showHelp();

    /**
     * @brief Show the version information and terminate the program.
     */
    [[noreturn]] static void showVersion();

    CompileOptions& m_options; ///< Reference to a CompileOptions object where the parsed options will be stored.
};

} // namespace lbc