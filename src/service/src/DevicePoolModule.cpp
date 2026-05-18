// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DevicePoolModule.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>
#include <vector>

using DevicePoolModule = wiregate::DevicePoolModule;

namespace {

wiregate::DeviceInfo makeThermostat(std::chrono::system_clock::time_point now) {
    return wiregate::DeviceInfo{
        .id = "thermostat-001",
        .name = "Living Room Thermostat",
        .type = "thermostat",
        .location = "Living Room",
        .status = "online",
        .lastSeen = now,
        .telemetry = {
            {"temperature", 21.0},
            {"humidity", 45},
            {"targetTemperature", 22.0},
            {"heating", false}
        },
        .supportedCommands = {"restart", "eco_mode", "comfort_mode"},
        .settings = {
            {"targetTemperature", 22.0},
            {"ecoTemperature", 19.0},
            {"comfortTemperature", 22.0},
            {"sampleIntervalSec", 5}
        },
        .lastCommand = {}
    };
}

wiregate::DeviceInfo makeRelay(std::chrono::system_clock::time_point now) {
    return wiregate::DeviceInfo{
        .id = "relay-001",
        .name = "Garage Relay",
        .type = "relay",
        .location = "Garage",
        .status = "off",
        .lastSeen = now,
        .telemetry = {
            {"state", "off"}
        },
        .supportedCommands = {"turn_on", "turn_off", "restart"},
        .settings = {
            {"defaultState", "off"},
            {"autoOffSec", 0}
        },
        .lastCommand = {}
    };
}

wiregate::DeviceInfo makePowerMeter(std::chrono::system_clock::time_point now) {
    return wiregate::DeviceInfo{
        .id = "meter-001",
        .name = "Main Power Meter",
        .type = "power_meter",
        .location = "Utility Room",
        .status = "online",
        .lastSeen = now,
        .telemetry = {
            {"voltage", 230},
            {"power", 120},
            {"energy", 12.4}
        },
        .supportedCommands = {"restart", "reset_energy"},
        .settings = {
            {"sampleIntervalSec", 2},
            {"reportThresholdW", 25}
        },
        .lastCommand = {}
    };
}

wiregate::DeviceInfo makeDoorSensor(std::chrono::system_clock::time_point now) {
    return wiregate::DeviceInfo{
        .id = "door-sensor-001",
        .name = "Front Door Sensor",
        .type = "contact_sensor",
        .location = "Entrance",
        .status = "online",
        .lastSeen = now,
        .telemetry = {
            {"open", false},
            {"battery", 91},
            {"signal", 82}
        },
        .supportedCommands = {"restart", "calibrate"},
        .settings = {
            {"normallyOpen", false},
            {"lowBatteryThreshold", 20},
            {"reportIntervalSec", 30}
        },
        .lastCommand = {}
    };
}

wiregate::DeviceInfo makeLeakSensor(std::chrono::system_clock::time_point now) {
    return wiregate::DeviceInfo{
        .id = "leak-sensor-001",
        .name = "Basement Leak Sensor",
        .type = "leak_sensor",
        .location = "Basement",
        .status = "online",
        .lastSeen = now,
        .telemetry = {
            {"waterDetected", false},
            {"battery", 77},
            {"humidity", 52}
        },
        .supportedCommands = {"restart", "silence_alarm"},
        .settings = {
            {"alarmEnabled", true},
            {"sensitivity", "high"},
            {"lowBatteryThreshold", 20}
        },
        .lastCommand = {}
    };
}

wiregate::DeviceInfo makeAirQualityMonitor(std::chrono::system_clock::time_point now) {
    return wiregate::DeviceInfo{
        .id = "air-quality-001",
        .name = "Office Air Quality",
        .type = "air_quality",
        .location = "Office",
        .status = "online",
        .lastSeen = now,
        .telemetry = {
            {"co2", 620},
            {"pm25", 7},
            {"voc", 120}
        },
        .supportedCommands = {"restart", "calibrate"},
        .settings = {
            {"sampleIntervalSec", 10},
            {"co2WarningPpm", 1000},
            {"pm25Warning", 35}
        },
        .lastCommand = {}
    };
}

wiregate::DeviceInfo makeSmartPlug(std::chrono::system_clock::time_point now) {
    return wiregate::DeviceInfo{
        .id = "plug-001",
        .name = "Kitchen Smart Plug",
        .type = "smart_plug",
        .location = "Kitchen",
        .status = "on",
        .lastSeen = now,
        .telemetry = {
            {"state", "on"},
            {"power", 48},
            {"energy", 2.1}
        },
        .supportedCommands = {"turn_on", "turn_off", "restart", "reset_energy"},
        .settings = {
            {"maxPowerW", 1800},
            {"overloadProtection", true},
            {"sampleIntervalSec", 5}
        },
        .lastCommand = {}
    };
}

void ensureDevice(const wiregate::DeviceManager::Ptr& manager, wiregate::DeviceInfo device) {
    if (!manager) {
        return;
    }

    if (auto existing = manager->find(device.id)) {
        existing->lastSeen = device.lastSeen;
        manager->updateOrInsert(std::move(*existing));
        return;
    }

    manager->updateOrInsert(std::move(device));
}

bool supportsCommand(const wiregate::DeviceInfo& device, const std::string& command) {
    return std::find(device.supportedCommands.begin(), device.supportedCommands.end(), command) != device.supportedCommands.end();
}

void applyCommand(wiregate::DeviceInfo& device) {
    if (device.lastCommand.empty() || !supportsCommand(device, device.lastCommand)) {
        return;
    }

    if (device.lastCommand == "turn_on") {
        device.status = "on";
    } else if (device.lastCommand == "turn_off") {
        device.status = "off";
    } else if (device.lastCommand == "restart") {
        device.status = "online";
    } else if (device.lastCommand == "eco_mode") {
        device.settings["targetTemperature"] = device.settings.value("ecoTemperature", 19.0);
    } else if (device.lastCommand == "comfort_mode") {
        device.settings["targetTemperature"] = device.settings.value("comfortTemperature", 22.0);
    } else if (device.lastCommand == "silence_alarm") {
        device.status = "online";
    } else if (device.lastCommand == "reset_energy") {
        device.settings["energyReset"] = true;
    } else if (device.lastCommand == "calibrate") {
        device.settings["calibrated"] = true;
    }

    device.lastCommand.clear();
}

} // namespace

// ############################################################
// DevicePoolModule implementation

// It will be a "fake" module, as IoT communication is complicated
// And to make this hub concept to work - data about devices will be faked
// Subject to improve infuture, when moving forward from simple concept of hub
// to something, that really works with IoT devices

DevicePoolModule::DevicePoolModule() {

}

DevicePoolModule::~DevicePoolModule() {

}

void DevicePoolModule::initialize(const ServiceSettings& settings) {
    (void)settings;
    tick = 0;
}

void DevicePoolModule::start(const ServiceContext& context) {
    const auto now = std::chrono::system_clock::now();

    ensureDevice(context.deviceManager, makeThermostat(now));
    ensureDevice(context.deviceManager, makeRelay(now));
    ensureDevice(context.deviceManager, makePowerMeter(now));
    ensureDevice(context.deviceManager, makeDoorSensor(now));
    ensureDevice(context.deviceManager, makeLeakSensor(now));
    ensureDevice(context.deviceManager, makeAirQualityMonitor(now));
    ensureDevice(context.deviceManager, makeSmartPlug(now));
}

void DevicePoolModule::execute(const ServiceContext& context) {
    if (!context.deviceManager) {
        return;
    }

    ++tick;
    const auto now = std::chrono::system_clock::now();

    for (auto device : context.deviceManager->list()) {
        applyCommand(device);

        if (device.type == "thermostat") {
            const auto temperature = 21.0 + static_cast<double>(tick % 10) * 0.2;
            const auto targetTemperature = device.settings.value("targetTemperature", 22.0);

            device.telemetry = {
                {"temperature", temperature},
                {"humidity", 40 + static_cast<int>(tick % 15)},
                {"targetTemperature", targetTemperature},
                {"heating", temperature < targetTemperature - 0.2}
            };
        } else if (device.type == "relay") {
            device.telemetry = {
                {"state", device.status}
            };
        } else if (device.type == "power_meter") {
            const auto baseEnergy = device.settings.value("energyReset", false) ? 0.0 : 12.4;
            device.telemetry = {
                {"voltage", 230},
                {"power", 100 + static_cast<int>((tick * 7) % 80)},
                {"energy", baseEnergy + static_cast<double>(tick % 100) * 0.03}
            };
        } else if (device.type == "contact_sensor") {
            device.telemetry = {
                {"open", (tick / 8) % 2 == 1},
                {"battery", 91 - static_cast<int>(tick % 9)},
                {"signal", 82 - static_cast<int>(tick % 6)}
            };
        } else if (device.type == "leak_sensor") {
            const bool waterDetected = (tick % 24) == 0;
            if (waterDetected) {
                device.status = "warning";
            } else if (device.status == "warning") {
                device.status = "online";
            }

            device.telemetry = {
                {"waterDetected", waterDetected},
                {"battery", 77 - static_cast<int>(tick % 5)},
                {"humidity", 52 + static_cast<int>(tick % 8)}
            };
        } else if (device.type == "air_quality") {
            device.telemetry = {
                {"co2", 620 + static_cast<int>((tick * 17) % 260)},
                {"pm25", 7 + static_cast<int>(tick % 6)},
                {"voc", 120 + static_cast<int>((tick * 9) % 80)}
            };
        } else if (device.type == "smart_plug") {
            const bool active = device.status != "off";
            const auto baseEnergy = device.settings.value("energyReset", false) ? 0.0 : 2.1;
            device.telemetry = {
                {"state", active ? "on" : "off"},
                {"power", active ? 40 + static_cast<int>((tick * 5) % 35) : 0},
                {"energy", active ? baseEnergy + static_cast<double>(tick % 40) * 0.02 : baseEnergy}
            };
        }

        device.lastSeen = now;
        context.deviceManager->updateOrInsert(std::move(device));
    }

}
    
void DevicePoolModule::shutdown() {

}
