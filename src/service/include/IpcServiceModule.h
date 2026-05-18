// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Service.h"
#include "IpcServer.h"

#include <atomic>
#include <memory>
#include <mutex>

// ############################################################

namespace wiregate::ipc {
// Forward declaration of the IpcServer class
// to avoid include with deep denepndecies
class IpcServer;
}


namespace wiregate {

// ############################################################
// IpcServiceModule class

class IpcServiceModule : public ServiceModule {
public:
    IpcServiceModule();
    ~IpcServiceModule() override; 

    void initialize(const ServiceSettings& settings) override;
    void start(const ServiceContext& context) override;
    void execute(const ServiceContext& context) override;
    void shutdown() override;

private:
    ipc::IpcResponse handleRequest(const ipc::IpcRequest& request);

private:
    std::atomic_bool running;
    ipc::IpcServer::Ptr ipcServer;
    std::mutex deviceManagerMutex;
    DeviceManager::Ptr deviceManager;
};

} // namespace wiregate
