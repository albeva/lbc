//
// Created by Albert Varaksin on 25/05/2021.
//
#pragma once
#include "pch.hpp"

/**
 * @file ControlFlowStack.hpp
 * @brief This file contains the ControlFlowStack class which is a helper class to manage nested control structures.
 */

namespace lbc {

/**
 * @enum ControlFlowStatement
 * @brief Enum class for different types of control flow statements.
 */
enum class ControlFlowStatement : uint8_t {
    For, ///< For loop control flow statement
    Do   ///< Do loop control flow statement
};

/**
 * @class ControlFlowStack
 * @brief Helper class to manage nested control structures.
 *
 * This class acts as a stack, we can push & pop structures from it.
 * Each control structure can have data (e.g. control exit and continue labels).
 *
 * @tparam Data The type of data to be stored in the control flow stack. Default is std::monostate.
 */
template<typename Data = std::monostate>
    requires(std::is_trivial_v<Data>)
class ControlFlowStack final {
    /**
     * @brief Type alias for the entry in the control flow stack.
     */
    using Entry = std::pair<ControlFlowStatement, Data>;

    /**
     * @brief Type alias for the container used to store the entries.
     */
    using Container = std::vector<Entry>;
public:

    /**
     * @brief Type alias for the const reverse iterator.
     */
    using Iterator = typename Container::const_reverse_iterator;

    /**
     * @brief Push a control flow statement and its associated data onto the stack.
     *
     * @param control The control flow statement to be pushed onto the stack.
     * @param data The data associated with the control flow statement. Default is {}.
     */
    void push(ControlFlowStatement control, Data data = {}) {
        m_container.emplace_back(control, data);
    }

    /**
     * @brief Pop the top control flow statement and its associated data from the stack.
     */
    void pop() {
        m_container.pop_back();
    }

    /**
     * @brief Clear the stack.
     */
    void clear() {
        m_container.clear();
    }

    /**
     * @brief Check if the stack is empty.
     *
     * @return true if the stack is empty, false otherwise.
     */
    [[nodiscard]] auto empty() const -> bool {
        return m_container.empty();
    }

    /**
     * @brief Get the size of the stack.
     *
     * @return The number of control flow statements in the stack.
     */
    [[nodiscard]] auto size() const -> size_t {
        return m_container.size();
    }

    /**
     * @brief Get the index of the given iterator in the stack.
     *
     * @param iter The iterator to find the index of.
     * @return The index of the iterator in the stack.
     */
    [[nodiscard]] auto indexOf(Iterator iter) const -> size_t {
        return static_cast<size_t>(std::distance(m_container.begin(), iter.base()) - 1);
    }

    /**
     * @brief Get an iterator pointing to the element after the given index in the stack.
     *
     * @param index The index to get the iterator after.
     * @return An iterator pointing to the element after the given index in the stack.
     */
    [[nodiscard]] auto after(size_t index) const -> Iterator {
        auto iter = begin();
        std::advance(iter, size() - index);
        return iter;
    }

    /**
     * @brief Get the entry at the given index in the stack.
     *
     * @param index The index to get the entry at.
     * @return The entry at the given index in the stack.
     */
    [[nodiscard]] auto operator[](size_t index) -> Entry& {
        return m_container[index];
    }

    /**
     * @brief Execute a handler function with a given entry.
     *
     * This function pushes the given entry onto the stack, executes the handler function, and then pops the entry from the stack.
     *
     * @param entry The entry to be pushed onto the stack.
     * @param handler The handler function to be executed.
     * @return The result of executing the handler function.
     */
    auto with(Entry entry, std::invocable auto handler) {
        m_container.push_back(std::move(entry));
        DEFER { pop(); };
        return handler();
    }

    /**
     * @brief Execute a handler function with a given control flow statement.
     *
     * This function pushes the given control flow statement onto the stack, executes the handler function, and then pops the control flow statement from the stack.
     *
     * @param control The control flow statement to be pushed onto the stack.
     * @param handler The handler function to be executed.
     * @return The result of executing the handler function.
     */
    auto with(ControlFlowStatement control, std::invocable auto handler) {
        return with({ control, {} }, handler);
    }

    /**
     * @brief Find a given control flow statement in the stack from a given iterator.
     *
     * @param from The iterator to start the search from.
     * @param control The control flow statement to find.
     * @return An iterator pointing to the found control flow statement, or cend() if the control flow statement is not found.
     */
    [[nodiscard]] auto find(Iterator from, ControlFlowStatement control) const -> Iterator {
        return std::find_if(from, end(), [&](const auto& entry) {
            return entry.first == control;
        });
    }

    /**
     * @brief Get a const reverse iterator to the beginning of the stack.
     *
     * @return A const reverse iterator to the beginning of the stack.
     */
    [[nodiscard]] auto begin() const -> Iterator { return m_container.crbegin(); }

    /**
     * @brief Get a const reverse iterator to the end of the stack.
     *
     * @return A const reverse iterator to the end of the stack.
     */
    [[nodiscard]] auto end() const -> Iterator { return m_container.crend(); }

private:
    /***
     * The container used to store the entries in the stack.
     */
    Container m_container{};
};

} // namespace lbc