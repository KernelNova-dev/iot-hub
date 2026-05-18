// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "WireGateService.h"
#include "logger.h"

#include <thread>

// ############################################################

using WireGateService = wiregate::WireGateService;
using WireGateLOgger = wiregate::core::Logger;

// ############################################################

namespace /* anonymous */ {

WireGateLOgger logger = WireGateLOgger::getLogger("WireGateService");


} // namespace anonymous

// ############################################################
// WireGateService implementation

WireGateService::WireGateService() {
    running = false;
    deviceManager = std::make_shared<wiregate::DeviceRegistry>();
}

WireGateService::~WireGateService() {
    shutdown();
}

bool WireGateService::installModule(ServiceModule::Ptr module) {
    if (running) {
        logger.warn("Cannot add module when service is already running");
        return false;
    }
    if (module == nullptr) {
        logger.error("Cannot install/add module: null pointer");
        return false;
    }

    modules.push_back(module);
    return true;
}

bool WireGateService::startMainLoop(const ServiceSettings& settings) {
    if (running) {
        logger.warn("Cannot start service: it is already running");
        return false; // Already running
    }
    logger.info("Service starting");

    serviceSettings = settings;
    running = true;

    mainLoop();
    return true;
}

void WireGateService::shutdown() {
    running = false;
}

void WireGateService::mainLoop() {
    ServiceContext serviceContenxt;
    // Initially populate context, so all the modules could properly start
    refreshContext(serviceContenxt);

    // Initialize and start all modules
    initializeModules();
    startModules(serviceContenxt);

    logger.info("Starting main loop");
    while (running) {
       // Lifecycle loop
        refreshContext(serviceContenxt);
        refreshRuntimeState(serviceContenxt);
        executeModules(serviceContenxt);
        processServiceTasks(serviceContenxt);
        idleStateHandler();
    }
    logger.info("Exited main loop: shutdown requested");

    // Shutdown all modules
    shutdownModules();

    logger.info("Main loop shutdown complete");
}

void WireGateService::refreshContext(ServiceContext& context) {
    context.deviceManager = deviceManager;
    context.serviceSettings = &serviceSettings;
}

void WireGateService::initializeModules() {
    logger.debug("Initializing modules");
    size_t moduleCount = modules.size();
    for (size_t i = 0; i < moduleCount; i++) {
        modules[i]->initialize(serviceSettings);
    }
}

void WireGateService::startModules(const ServiceContext& context) {
    logger.debug("Starting modules");
    size_t moduleCount = modules.size();
    for (size_t i = 0; i < moduleCount; i++) {
        modules[i]->start(context);
    }
}

void WireGateService::shutdownModules() {
    logger.debug("Shutting down modules");
    size_t moduleCount = modules.size();
    for (size_t i = 0; i < moduleCount; i++) {
        modules[i]->shutdown();
    }
}

void WireGateService::refreshRuntimeState(const ServiceContext& context) {
    (void)context;
    // Nothing for now: added for etendability
}

void WireGateService::executeModules(const ServiceContext& context) {
    size_t moduleCount = modules.size();
    for (size_t i = 0; i < moduleCount; i++) {
        modules[i]->execute(context);
    }
}

void WireGateService::processServiceTasks(const ServiceContext& context) {
    (void)context;
    // Nothing for now: added for etendability
}

void WireGateService::idleStateHandler() {
    std::this_thread::sleep_for(serviceSettings.serviceIdleTimeout);
}
