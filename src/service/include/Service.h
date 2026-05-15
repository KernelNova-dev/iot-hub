#pragma once

#include <atomic>
#include <string>
#include <chrono>
#include <memory>
#include <vector>

namespace iothub {

// ############################################################
// ServiceSettings struct

struct ServiceSettings {
    std::string ipcSocketPath;
    std::chrono::milliseconds serviceIdleTimeout;
};

ServiceSettings loadServiceSettings(const std::string& configFilePath);

// ############################################################
// ServiceModule class

class ServiceModule {
public:
    using Ptr = std::shared_ptr<ServiceModule>;

    virtual ~ServiceModule() = default; 
    virtual void initialize() = 0;
    virtual void execute() = 0;
    virtual void shutdown() = 0;
};


// ############################################################
// Service class

class Service {
public:
    Service(const ServiceSettings& settings);
    ~Service();

    void start();
    void stop();

private:
    void mainLoop();    


private:
    ServiceSettings settings;
    std::vector<ServiceModule::Ptr> modules;
    std::atomic_bool running; 
};

} // namespace iothub
