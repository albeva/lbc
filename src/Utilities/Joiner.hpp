//
// Created by Albert Varaksin on 04/04/2026.
//
#pragma once

namespace lbc {
template<typename Output>
struct [[nodiscard]] Joiner final {
    NO_COPY_AND_MOVE(Joiner)

    constexpr explicit Joiner(Output& output, const std::string& separator = ", ")
    : m_output(output)
    , m_separator(separator)
    , m_isFirst(true) {}

    constexpr void operator()() {
        if (m_isFirst) {
            m_isFirst = false;
        } else {
            append();
        }
    }

private:
    constexpr void append()
        requires requires(Output& out, std::string str) { out += str; }
    {
        m_output += m_separator;
    }

    constexpr void append()
        requires requires(Output& out, std::string str) { out << str; }
    {
        m_output << m_separator;
    }

    Output& m_output;
    std::string m_separator;
    bool m_isFirst;
};
} // namespace lbc
