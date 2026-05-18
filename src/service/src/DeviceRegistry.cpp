// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DeviceRegistry.h"

#include <algorithm>
#include <chrono>
#include <utility>

namespace wiregate {

DeviceRegistry::DeviceRegistry() = default;

DeviceRegistry::DeviceRegistry(DeviceRegistry&& other) noexcept {
    std::lock_guard lock(other.mutex);
    devices = std::move(other.devices);
}

DeviceRegistry::~DeviceRegistry() = default;

void DeviceRegistry::updateOrInsert(DeviceInfo device) {
    if (device.id.empty()) {
        return;
    }

    if (device.lastSeen == std::chrono::system_clock::time_point{}) {
        device.lastSeen = std::chrono::system_clock::now();
    }

    std::lock_guard lock(mutex);
    devices[device.id] = std::move(device);
}

std::vector<DeviceInfo> DeviceRegistry::list() const {
    std::vector<DeviceInfo> result;

    {
        std::lock_guard lock(mutex);
        result.reserve(devices.size());

        for (const auto& [_, device] : devices) {
            result.push_back(device);
        }
    }

    return result;
}

std::optional<DeviceInfo> DeviceRegistry::find(const std::string& id) const {
    std::lock_guard lock(mutex);

    const auto it = devices.find(id);
    if (it == devices.end()) {
        return std::nullopt;
    }

    return it->second;
}

bool DeviceRegistry::updateTelemetry(const std::string& id, nlohmann::json telemetry) {
    std::lock_guard lock(mutex);

    const auto it = devices.find(id);
    if (it == devices.end()) {
        return false;
    }

    it->second.telemetry = std::move(telemetry);
    it->second.lastSeen = std::chrono::system_clock::now();
    return true;
}

bool DeviceRegistry::setStatus(const std::string& id, std::string status) {
    std::lock_guard lock(mutex);

    const auto it = devices.find(id);
    if (it == devices.end()) {
        return false;
    }

    it->second.status = std::move(status);
    it->second.lastSeen = std::chrono::system_clock::now();
    return true;
}

bool DeviceRegistry::sendCommand(const std::string& id, std::string command) {
    std::lock_guard lock(mutex);

    const auto it = devices.find(id);
    if (it == devices.end()) {
        return false;
    }

    it->second.lastCommand = std::move(command);
    return true;
}

} // namespace wiregate
