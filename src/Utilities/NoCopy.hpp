//
// Created by Albert Varaksin on 08/07/2025.
//
#pragma once

#define NO_COPY_AND_MOVE(Class)               \
    Class(Class&&) = delete;                  \
    Class(const Class&) = delete;             \
    auto operator=(Class&&)->Class& = delete; \
    auto operator=(const Class&)->Class& = delete;
