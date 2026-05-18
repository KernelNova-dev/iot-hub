// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <optional>
#include <vector>

#include <nlohmann/json.hpp>

namespace wiregate {

struct DeviceInfo {
    std::string id;
    std::string name;
    std::string type;
    std::string location;
    std::string status;
    std::chrono::system_clock::time_point lastSeen;
    nlohmann::json telemetry;
    std::vector<std::string> supportedCommands;
    nlohmann::json settings;
    std::string lastCommand;
};

class DeviceManager {
public:
    using Ptr = std::shared_ptr<DeviceManager>;

    virtual ~DeviceManager() = default;

    virtual std::vector<DeviceInfo> list() const = 0;
    virtual std::optional<DeviceInfo> find(const std::string& id) const = 0;

    virtual void updateOrInsert(DeviceInfo device) = 0;
    virtual bool updateTelemetry(const std::string& id, nlohmann::json telemetry) = 0;
    virtual bool setStatus(const std::string& id, std::string status) = 0;
    virtual bool sendCommand(const std::string& id, std::string command) = 0;
};

} // namespace wiregate
