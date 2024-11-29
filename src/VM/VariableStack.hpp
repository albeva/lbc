//
// Created by Albert Varaksin on 28/11/2024.
//
#pragma once
#include "pch.hpp"

namespace lbc {

/**
 * A simple stack to store values or variable sizes
 */
class VariableStack final {
    NO_COPY_AND_MOVE(VariableStack)
public:
    VariableStack() {
        m_storage.reserve(defaultSize);
        m_storage.resize(defaultSize);
    }

    ~VariableStack() = default;

    /**
     * Push the value onto the stack.
     *
     * @param value The value to push.
     */
    template <std::semiregular T>
    void push(const T value) {
        const auto size = sizeof(T);
        if (m_offset + size > m_storage.size()) {
            const auto newSize = m_storage.size() + defaultSize;
            m_storage.reserve(newSize);
            m_storage.resize(newSize);
        }
        auto* ptr = &m_storage[m_offset];
        std::memcpy(ptr, &value, size);
        m_offset += size;
    }

    /**
     * Peek the value from the stack.
     *
     * @return The value peeked from the stack.
     */
    template <std::semiregular T>
    [[nodiscard]] auto peek() const -> T {
        const auto size = sizeof(T);
        const auto offset = m_offset - size;
        const auto* ptr = &m_storage[offset];
        T value;
        std::memcpy(&value, ptr, size);
        return value;
    }

    /**
     * Pop the value from the stack.
     *
     * @return The value popped from the stack.
     */
    template <std::semiregular T>
    [[nodiscard]] auto pop() -> T {
        const auto size = sizeof(T);
        m_offset -= size;
        const auto* ptr = &m_storage[m_offset];
        T value;
        std::memcpy(&value, ptr, size);
        return value;
    }

private:
    constexpr static size_t defaultSize = 128;
    size_t m_offset { 0 };
    std::vector<std::byte> m_storage;
};
} // namespace lbc
