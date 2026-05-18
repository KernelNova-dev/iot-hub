// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Device.h"

namespace wiregate {

// ############################################################
// DeviceRegistry

class DeviceRegistry : public DeviceManager {
public:

    DeviceRegistry();
    DeviceRegistry(const DeviceRegistry&) = delete;
    DeviceRegistry& operator=(const DeviceRegistry&) = delete;
    DeviceRegistry(DeviceRegistry&& other) noexcept;
    ~DeviceRegistry();

    void updateOrInsert(DeviceInfo device);
    std::vector<DeviceInfo> list() const;
    std::optional<DeviceInfo> find(const std::string& id) const;

    bool updateTelemetry(const std::string& id, nlohmann::json telemetry);
    bool setStatus(const std::string& id, std::string status);
    bool sendCommand(const std::string& id, std::string command);

private:
    mutable std::mutex mutex; // protects for multi-threaded operations
    std::unordered_map<std::string, DeviceInfo> devices;
};

} // namespace wiregate
