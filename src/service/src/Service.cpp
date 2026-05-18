// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Service.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <thread>

// ############################################################

wiregate::ServiceSettings wiregate::loadServiceSettings(const std::string& configFilePath) {
    // Load JSON file and parse settings
    nlohmann::json config;
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open config file: " + configFilePath);
    }
    configFile >> config;

    wiregate::ServiceSettings settings;
    settings.ipcSocketPath = config.value("ipcSocketPath", "/tmp/wiregate_service.sock");
    settings.serviceIdleTimeout = std::chrono::milliseconds(config.value("serviceIdleTimeout", 30000));

    return settings;
}

// ############################################################
