// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <string>
#include <chrono>
#include <memory>
#include <vector>

#include "Device.h"

namespace wiregate {

// ############################################################
// ServiceSettings struct

struct ServiceSettings {
    std::string ipcSocketPath;
    std::chrono::milliseconds serviceIdleTimeout;
};

ServiceSettings loadServiceSettings(const std::string& configFilePath);

// ############################################################
// ServiceContext

struct ServiceContext {
    const ServiceSettings* serviceSettings{nullptr};
    DeviceManager::Ptr deviceManager;
};

// ############################################################
// ServiceModule class

class ServiceModule {
public:
    using Ptr = std::shared_ptr<ServiceModule>;

    virtual ~ServiceModule() = default; 
    virtual void initialize(const ServiceSettings& settings) = 0;
    virtual void start(const ServiceContext&) {}
    virtual void execute(const ServiceContext&) {}
    virtual void shutdown() = 0;
};

// ############################################################
// Service abstract interface

class Service {
public:
    virtual ~Service() = default;

    virtual bool installModule(ServiceModule::Ptr module) = 0;
    virtual bool startMainLoop(const ServiceSettings& settings) = 0;
    virtual void shutdown() = 0;
};

} // namespace wiregate
