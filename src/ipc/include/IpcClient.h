#pragma once

#include "IpcMessage.h"

#include <asio.hpp>

#include <chrono>
#include <string>

namespace iothub::ipc {

class IpcClient {
public:
    explicit IpcClient(
        std::string socketPath,
        std::chrono::milliseconds requestTimeout = std::chrono::seconds{5}
    );

    IpcResponse request(const IpcRequest& request);

private:
    std::string socketPath_;
    std::chrono::milliseconds requestTimeout_;
};

} // namespace iothub::ipc
