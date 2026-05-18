// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace wiregate::ipc {

struct IpcRequest {
    std::string type;
    std::string deviceId;
    std::string command;

    [[nodiscard]] std::string serialize() const;
    [[nodiscard]] static IpcRequest deserialize(std::string_view jsonText);
};

struct IpcResponse {
    bool ok{true};
    std::string message;
    nlohmann::json payload{nullptr};

    [[nodiscard]] std::string serialize() const;
    [[nodiscard]] static IpcResponse deserialize(std::string_view jsonText);

    [[nodiscard]] static IpcResponse success(nlohmann::json payload = nullptr);
    [[nodiscard]] static IpcResponse error(std::string message);
};

} // namespace wiregate::ipc
