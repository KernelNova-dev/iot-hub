// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "IpcServiceModule.h"
#include "logger.h"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <utility>

using IpcServiceModule = wiregate::IpcServiceModule;
using IpcServer = wiregate::ipc::IpcServer;
using IpcRequest = wiregate::ipc::IpcRequest;
using IpcResponse = wiregate::ipc::IpcResponse;
using Logger = wiregate::core::Logger;

// ############################################################
// Helpers and Globals

namespace /* anonymous */ {

// Module-wide logger
Logger logger = Logger::getLogger("IpcServiceModule");

nlohmann::json deviceToJson(const wiregate::DeviceInfo& device) {
    return {
        {"id", device.id},
        {"name", device.name},
        {"type", device.type},
        {"location", device.location},
        {"status", device.status},
        {"telemetry", device.telemetry},
        {"supportedCommands", device.supportedCommands},
        {"settings", device.settings},
        {"lastCommand", device.lastCommand}
    };
}

} // namespace anonymous

// ############################################################
// IpcServiceModule implementation

IpcServiceModule::IpcServiceModule() {
    running = false;
}

IpcServiceModule::~IpcServiceModule() {
    shutdown();
}

void IpcServiceModule::initialize(const ServiceSettings& settings) {
    if (running) {
        logger.warn("Cannot initialize module: its already is running");
        return;
    }

    ipcServer = std::make_shared<IpcServer>(settings.ipcSocketPath);
    ipcServer->setHandler([this](const IpcRequest& request){
        return handleRequest(request);
    });
}

void IpcServiceModule::start(const ServiceContext& context) {
    if (running) {
        logger.warn("Cannot start module: its already is running");
        return;
    }

    if (ipcServer == nullptr) {
        logger.error("Cannot start IPC module: IPC server was not initialized");
        return;
    }

    // Caching device manager for future use
    {
        std::lock_guard lock(deviceManagerMutex);
        deviceManager = context.deviceManager;
    }

    running = ipcServer->start();
}

void IpcServiceModule::execute(const ServiceContext& context) {
    if (!running) {
        logger.error("Cannot process IPC requests: IPC services was not started");
        return;
    }
    if (ipcServer == nullptr) {
        logger.error("Cannot process IPC requests: IPC services was not created/initialized");
    }

    if (deviceManager != context.deviceManager) {
        logger.info("DeviceManager instance changed: updating reference");
        std::lock_guard lock(deviceManagerMutex);
        deviceManager = context.deviceManager;
    }

    // This module is totally async and does its work mostly in the IPC event handler.
    // This was done to make it as responsive as possible.
}

void IpcServiceModule::shutdown() {
    if (ipcServer) {
        ipcServer->stop();
    }
    running = false;
}

IpcResponse IpcServiceModule::handleRequest(const IpcRequest& request) {
    DeviceManager::Ptr manager;
    {
        std::lock_guard lock(deviceManagerMutex);
        manager = deviceManager;
    }

    if (!manager) {
        logger.error("Cannot handle IPC request: device manager is null");
        return IpcResponse::error("Device manager is not available");
    }

    if (request.type == "list-devices") {
        nlohmann::json devices = nlohmann::json::array();
        for (const auto& device : manager->list()) {
            devices.push_back(deviceToJson(device));
        }

        return IpcResponse::success({{"devices", std::move(devices)}});
    }

    if (request.type == "get-device") {
        if (request.deviceId.empty()) {
            return IpcResponse::error("deviceId is required");
        }

        const auto device = manager->find(request.deviceId);
        if (!device) {
            return IpcResponse::error("Device not found: " + request.deviceId);
        }

        return IpcResponse::success({{"device", deviceToJson(*device)}});
    }

    if (request.type == "send-command") {
        if (request.deviceId.empty()) {
            return IpcResponse::error("deviceId is required");
        }
        if (request.command.empty()) {
            return IpcResponse::error("command is required");
        }

        const auto device = manager->find(request.deviceId);
        if (!device) {
            return IpcResponse::error("Device not found: " + request.deviceId);
        }

        const auto commandIt = std::find(
            device->supportedCommands.begin(),
            device->supportedCommands.end(),
            request.command
        );

        if (commandIt == device->supportedCommands.end()) {
            return IpcResponse::error("Unsupported command '" + request.command + "' for device: " + request.deviceId);
        }

        if (!manager->sendCommand(request.deviceId, request.command)) {
            return IpcResponse::error("Device not found: " + request.deviceId);
        }

        return IpcResponse::success({
            {"deviceId", request.deviceId},
            {"command", request.command}
        });
    }

    if (request.type == "service-status") {
        return IpcResponse::success({
            {"running", running.load()},
            {"deviceCount", manager->list().size()}
        });
    }

    return IpcResponse::error("Unsupported IPC request type: " + request.type);
}
