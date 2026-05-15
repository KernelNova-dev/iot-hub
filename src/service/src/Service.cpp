#include "Service.h"

#include <nlohmann/json.hpp>
#include <fstream>

// ############################################################

using namespace iothub;

ServiceSettings iothub::loadServiceSettings(const std::string& configFilePath) {
    // Load JSON file and parse settings
    nlohmann::json config;
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open config file: " + configFilePath);
    }
    configFile >> config;

    ServiceSettings settings;
    settings.ipcSocketPath = config.value("ipcSocketPath", "/tmp/iothub_service.sock");
    settings.serviceIdleTimeout = std::chrono::milliseconds(config.value("serviceIdleTimeout", 30000));

    return settings;
}

// ############################################################

Service::Service(const ServiceSettings& settings) : settings(settings) {
    running = false;
}

Service::~Service() {
    stop();
}

void Service::start() {
    if (running) {
        return; // Already running
    }

    running = true;

    mainLoop();
}

void Service::stop() {
    running = false;
}

void Service::mainLoop() {

    // Initialize all modules
    size_t moduleCount = modules.size();
    for (size_t i = 0; i < moduleCount; i++) {
        modules[i]->initialize();
    }

    // Main loop
    while (running) {

        // Poll devices and register new ones, 
        // old ones will be removed if they are not seen for a while
        pollDevices();

        // Update devices and send telemetry
        updateDevices();

        // Lets all modules do their work
        for (size_t i = 0; i < moduleCount; i++) {
            modules[i]->execute();
        }

        std::this_thread::sleep_for(settings.serviceIdleTimeout);
    }

    // Shutdown all modules
    for (size_t i = 0; i < moduleCount; i++) {
        modules[i]->shutdown();
    }
}

