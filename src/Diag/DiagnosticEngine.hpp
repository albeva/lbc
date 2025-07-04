//
// Created by Albert on 03/06/2021.
//
#pragma once
#include "pch.hpp"
#include "Diagnostics.def.hpp"

namespace lbc {
class Context;

/**
 * @enum Diag
 * @brief Enum class Diag that defines all the diagnostic messages.
 *
 * clang-format off
 */
enum class Diag : std::uint8_t {
#define DIAG(ID, ...) ID,
    ALL_ERRORS(DIAG)
#undef DIAG
};
// clang-format on

/**
 * @class DiagnosticEngine
 * @brief The DiagnosticEngine class is responsible for managing and reporting diagnostic messages.
 */
class DiagnosticEngine final {
    NO_COPY_AND_MOVE(DiagnosticEngine)
public:
    /**
     * @brief Constructor that takes a reference to a Context object.
     * @param context Reference to a Context object.
     */
    explicit DiagnosticEngine(Context& context);

    /**
     * @brief Default destructor.
     */
    ~DiagnosticEngine() = default;

    /**
     * @brief Method to check if there are any errors.
     * @return True if there are errors, false otherwise.
     */
    [[nodiscard]] auto hasErrors() const -> bool { return m_errorCounter > 0; }

    /**
     * @brief Method to ignore errors while executing a function.
     * @param func Function to be executed.
     * @return Result of the function execution.
     */
    auto ignoringErrors(std::invocable auto&& func) {
        RESTORE_ON_EXIT(m_ignoreErrors);
        m_ignoreErrors = true;
        return func();
    }

    /**
     * @brief Method to log diagnostic messages.
     * @tparam Args Variadic template for arguments.
     * @param diag Diagnostic message to log.
     * @param loc Location in the source code where the diagnostic message should be logged.
     * @param range Range in the source code where the diagnostic message should be logged.
     * @param args Arguments to format the diagnostic message.
     */
    template <typename... Args>
    void log(Diag diag, const llvm::SMLoc& loc, const llvm::SMRange& range, Args&&... args) {
        print(
            diag,
            loc,
            format(diag, std::forward<Args>(args)...),
            range
        );
    }

private:
    /**
     * @brief Method to get the text of a diagnostic message.
     * @param diag Diagnostic message.
     * @return Text of the diagnostic message.
     */
    static auto getText(Diag diag) -> const char*;

    /**
     * @brief Method to get the kind of a diagnostic message.
     * @param diag Diagnostic message.
     * @return Kind of the diagnostic message.
     */
    static auto getKind(Diag diag) -> llvm::SourceMgr::DiagKind;

    /**
     * @brief Method to format a diagnostic message.
     * @tparam Args Variadic template for arguments.
     * @param diag Diagnostic message.
     * @param args Arguments to format the diagnostic message.
     * @return Formatted diagnostic message.
     */
    template <typename... Args>
    static auto format(Diag diag, Args&&... args) -> std::string {
        auto str = llvm::formatv(getText(diag), std::forward<Args>(args)...).str();
        return str;
    }

    /**
     * @brief Method to print a diagnostic message.
     * @param diag Diagnostic message.
     * @param loc Location in the source code where the diagnostic message should be logged.
     * @param str Formatted diagnostic message.
     * @param ranges Ranges in the source code where the diagnostic message should be logged.
     */
    void print(Diag diag, llvm::SMLoc loc, const std::string& str, llvm::ArrayRef<llvm::SMRange> ranges = {});

    Context& m_context; ///< Reference to a Context object.
    int m_errorCounter = 0; ///< Counter for the number of errors.
    bool m_ignoreErrors = false; ///< Flag to indicate if errors should be ignored.
};

/**
 * @brief Concept to check if a class is provides a source range
 * @tparam T Class to be checked.
 */
template <class T>
concept RangeAware = requires(T base) {
    { base.getRange() } -> std::same_as<llvm::SMRange>;
};

/**
 * @struct ErrorLogger
 * @brief Struct to log errors.
 */
struct ErrorLogger {
    NO_COPY_AND_MOVE(ErrorLogger)

    /**
     * @brief Constructor that takes a reference to a DiagnosticEngine object.
     * @param diag Reference to a DiagnosticEngine object.
     */
    explicit ErrorLogger(DiagnosticEngine& diag)
    : m_diag { diag } {
    }

    /**
     * @brief Default destructor.
     */
    virtual ~ErrorLogger() = default;

    /**
     * @brief Method to log an error with a range.
     * @tparam T Type of the location.
     * @tparam Args Variadic template for arguments.
     * @param diag Diagnostic message.
     * @param range Range provider that provides the range where the error should be logged.
     * @param args Arguments to format the error message.
     * @return ResultError object.
     */
    template <RangeAware T, typename... Args>
    auto makeError(Diag diag, const T& range, Args&&... args) const -> ResultError {
        m_diag.log(diag, range.getRange().Start, range.getRange(), std::forward<Args>(args)...);
        return {};
    }

    /**
     * @brief Method to log an error with a pointer to a range.
     * @tparam T Type of the location.
     * @tparam Args Variadic template for arguments.
     * @param diag Diagnostic message.
     * @param range Range provider that provides the range where the error should be logged.
     * @param args Arguments to format the error message.
     * @return ResultError object.
     */
    template <RangeAware T, typename... Args>
    auto makeError(Diag diag, T* range, Args&&... args) const -> ResultError {
        return makeError(diag, *range, std::forward<Args>(args)...);
    }

    /**
     * @brief Method to log an error with a location and a range.
     * @tparam Args Variadic template for arguments.
     * @param diag Diagnostic message.
     * @param loc Location in the source code where the error should be logged.
     * @param range Range in the source code where the error should be logged.
     * @param args Arguments to format the error message.
     * @return ResultError object.
     */
    template <typename... Args>
    auto makeError(Diag diag, const llvm::SMLoc& loc, const llvm::SMRange& range, Args&&... args) const -> ResultError {
        m_diag.log(diag, loc, range, std::forward<Args>(args)...);
        return {};
    }

    /**
     * @brief Method to get the DiagnosticEngine object.
     * @return Reference to the DiagnosticEngine object.
     */
    [[nodiscard]] auto getDiag() const -> DiagnosticEngine& { return m_diag; }

private:
    DiagnosticEngine& m_diag; ///< Reference to a DiagnosticEngine object.
};

} // namespace lbc