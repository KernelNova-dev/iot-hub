// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "IpcMessage.h"

#include <utility>

namespace wiregate::ipc {

std::string IpcRequest::serialize() const {
    nlohmann::json json{
        {"type", type},
        {"deviceId", deviceId},
        {"command", command}
    };

    return json.dump();
}

IpcRequest IpcRequest::deserialize(std::string_view jsonText) {
    const auto json = nlohmann::json::parse(jsonText);

    return IpcRequest{
        .type = json.value("type", ""),
        .deviceId = json.value("deviceId", ""),
        .command = json.value("command", "")
    };
}

std::string IpcResponse::serialize() const {
    nlohmann::json json{
        {"status", ok ? "ok" : "error"}
    };

    if (ok) {
        json["payload"] = payload;
    } else {
        json["message"] = message;
    }

    return json.dump();
}

IpcResponse IpcResponse::deserialize(std::string_view jsonText) {
    const auto json = nlohmann::json::parse(jsonText);
    const auto status = json.value("status", "error");

    if (status == "ok") {
        return success(json.value("payload", nlohmann::json{nullptr}));
    }

    return error(json.value("message", "Unknown IPC error"));
}

IpcResponse IpcResponse::success(nlohmann::json payload) {
    return IpcResponse{
        .ok = true,
        .message = {},
        .payload = std::move(payload)
    };
}

IpcResponse IpcResponse::error(std::string message) {
    return IpcResponse{
        .ok = false,
        .message = std::move(message),
        .payload = nullptr
    };
}

} // namespace wiregate::ipc
