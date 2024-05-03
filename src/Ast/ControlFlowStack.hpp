//
// Created by Albert Varaksin on 25/05/2021.
//
#pragma once
#include "pch.hpp"


namespace lbc {

enum class ControlFlowStatement : uint8_t {
    For,
    Do
};

/**
 * Helper class to manage nested control structures.
 *
 * This class acts as a stack, we can push & pop structures from it
 *
 * Each control structure can have data (e.g. control exit and continue labels)
 */
template<typename Data = std::monostate> requires(std::is_trivial_v<Data>)
class ControlFlowStack final {
public:
    using Entry = std::pair<ControlFlowStatement, Data>;
    using Container = std::vector<Entry>;
    using const_iterator = typename Container::const_reverse_iterator;

    inline void push(ControlFlowStatement control, Data data = {}) {
        m_container.emplace_back(control, data);
    }

    inline void pop() {
        m_container.pop_back();
    }

    inline void clear() {
        m_container.clear();
    }

    [[nodiscard]] inline bool empty() const {
        return m_container.empty();
    }

    [[nodiscard]] inline size_t indexOf(const_iterator iter) const {
        return static_cast<size_t>(std::distance(m_container.begin(), iter.base()) - 1);
    }

    [[nodiscard]] inline size_t nextIndexAfter(const_iterator iter) const {
        return static_cast<size_t>(std::distance(m_container.begin(), iter.base()));
    }

    [[nodiscard]] inline Entry& operator[](size_t index) {
        return m_container[index];
    }

    template<std::invocable Func>
    inline auto with(Entry entry, Func handler) {
        m_container.push_back(std::move(entry));
        DEFER { pop(); };
        return handler();
    }

    template<std::invocable Func>
    inline auto with(ControlFlowStatement control, Func handler) {
        return with({ control, {} }, handler);
    }

    [[nodiscard]] const_iterator find(const_iterator from, ControlFlowStatement control) const {
        return std::find_if(from, cend(), [&](const auto& entry) {
            return entry.first == control;
        });
    }

    [[nodiscard]] inline const_iterator cbegin() const { return m_container.crbegin(); }
    [[nodiscard]] inline const_iterator cend() const { return m_container.crend(); }

private:
    Container m_container{};
};

} // namespace lbc
