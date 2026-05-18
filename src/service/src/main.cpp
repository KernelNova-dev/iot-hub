// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <atomic>
#include <cstdlib>
#include <csignal>
#include <memory>
#include <string>

#include "WireGateService.h"
#include "IpcServiceModule.h"
#include "DevicePoolModule.h"

using ServiceModule = wiregate::ServiceModule;
using WireGateService = wiregate::WireGateService;
using ServiceSettings = wiregate::ServiceSettings;
using IpcServiceModule = wiregate::IpcServiceModule;
using DevicePoolModule = wiregate::DevicePoolModule;

namespace {

WireGateService::Ptr service;

void handleSignal(int) {
    if (service != nullptr) {
        service->shutdown();
    }
}

std::string resolveConfigFilePath(int argc, char* argv[]) {
    if (argc > 1) {
        return argv[1];
    }

    if (const char* envPath = std::getenv("WIREGATE_CONFIG")) {
        return envPath;
    }

    return "config/wiregate-config.json";
}

} // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    std::string configFilePath = resolveConfigFilePath(argc, argv);
    ServiceSettings settings = wiregate::loadServiceSettings(configFilePath);
    service = std::make_shared<WireGateService>();

    // Create and install service modules
    ServiceModule::Ptr devicePoolModule = std::make_shared<DevicePoolModule>();
    service->installModule(devicePoolModule);

    ServiceModule::Ptr ipcModule = std::make_shared<IpcServiceModule>();
    service->installModule(ipcModule);


    //Start Service main loop
    bool result = service->startMainLoop(settings);

    if (!result) {
        return 1;
    }
    return 0;
}
