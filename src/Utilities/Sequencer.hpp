//
// Created by Albert Varaksin on 17/02/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc {
class Context;

/**
 * Satisfied by types that support intrusive singly-linked list chaining.
 */
template <typename T>
concept Sequencable = requires(T* node, T* next) {
    { node->setNext(next) } -> std::same_as<void>;
    { static_cast<T*>(node->getNext()) };
};

/**
 * A utility class to create a sequence of singly linked nodes and then
 * flatten pointers to those nodes into a contiguous buffer.
 *
 * @tparam T The type of node that has setNext() method.
 */
template <Sequencable T>
struct Sequencer final {
    /// Convenience alias for a pointer to the node type.
    using pointer = T*;

    /**
     * Add a node to the sequence by linking it to previous by setNext().
     *
     * @param node The AST node to add to the sequence.
     */
    void add(pointer node)  {
        size += 1;
        if (first == nullptr) {
            first = last = node;
        } else {
            last->setNext(node);
            last = node;
        }
    }

    /**
     * Append another sequence to this one.
     *
     * @param other The Sequencer to append.
     */
    void append(const Sequencer& other)  {
        if (other.size == 0) {
            return;
        }
        if (size == 0) {
            *this = other;
            return;
        }
        last->setNext(other.first);
        last = other.last;
        size += other.size;
    }

    /**
     * Append content of the span to this sequence
     */
    template<std::convertible_to<pointer> U>
    void append(std::span<U> nodes)  {
        for (auto* node : nodes) {
            add(node);
        }
    }

    /**
     * Convert the linked list sequence into a std::span<T*> by allocating
     * a contiguous buffer in the provided arena memory resource.
     * The node pointers are copied into the buffer in order of addition.
     *
     * @param context The memory arena to allocate the buffer from.
     * @returns A std::span<T*> containing all nodes in order.
     */
    [[nodiscard]] auto sequence(Context& context) const -> std::span<pointer> {
        if (size == 0) {
            return {};
        }

        auto span = context.span<pointer>(size);

        pointer node = first;
        for (pointer& el : span) {
            el = node;
            node = static_cast<T*>(node->getNext());
        }

        return span;
    }

private:
    /// Pointer to the head of the singly-linked list.
    pointer first {};

    /// Pointer to the tail of the singly-linked list.
    pointer last {};

    /// Number of elements added to the sequence.
    std::size_t size {};
};

} // namespace lbc
