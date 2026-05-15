#pragma once

#include "IpcMessage.h"

#include <asio.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace iothub::ipc {

class IpcServer {
public:
    using RequestHandler = std::function<IpcResponse(const IpcRequest& request)>;

    explicit IpcServer(std::string socketPath);
    ~IpcServer();

    IpcServer(const IpcServer&) = delete;
    IpcServer& operator=(const IpcServer&) = delete;

    void setHandler(RequestHandler handler);

    bool start();
    void stop();

private:
    void asyncAccept();

private:
    std::string socketPath_;

    asio::io_context io_;
    asio::local::stream_protocol::acceptor acceptor_;

    std::thread ioThread_;
    std::atomic_bool running_{false};

    RequestHandler handler_;
};

} // namespace iothub::ipc
