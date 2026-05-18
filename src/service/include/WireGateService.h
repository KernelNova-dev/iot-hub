// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Service.h"
#include "DeviceRegistry.h"

namespace wiregate {

// ############################################################
// WireGateService class

class WireGateService : public Service {
public:
    using Ptr = std::shared_ptr<WireGateService>;

    WireGateService();
    ~WireGateService() override;

    bool installModule(ServiceModule::Ptr module) override;
    bool startMainLoop(const ServiceSettings& settings) override;
    void shutdown() override;

private:
    void mainLoop();    

    void initializeModules();
    void startModules(const ServiceContext& context);
    void shutdownModules();

    void refreshContext(ServiceContext& context);

    void refreshRuntimeState(const ServiceContext& context);
    void executeModules(const ServiceContext& context);
    void processServiceTasks(const ServiceContext& context);
    void idleStateHandler();

private:
    ServiceSettings serviceSettings;
    DeviceManager::Ptr deviceManager;
    std::vector<ServiceModule::Ptr> modules;
    std::atomic_bool running; 
};


} // namespace wiregate