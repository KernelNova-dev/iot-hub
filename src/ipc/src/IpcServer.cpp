#include "IpcServer.h"
#include "IpcSession.h"

#include <filesystem>
#include <iostream>

namespace iothub::ipc {

namespace {

bool removeSocketFile(const std::string& socketPath) {
    std::error_code ec;
    const auto status = std::filesystem::symlink_status(socketPath, ec);

    if (ec || !std::filesystem::exists(status)) {
        return true;
    }

    if (!std::filesystem::is_socket(status)) {
        std::cerr << "[ipc] refusing to remove non-socket path: " << socketPath << '\n';
        return false;
    }

    std::filesystem::remove(socketPath, ec);
    if (ec) {
        std::cerr << "[ipc] failed to remove socket path: " << ec.message() << '\n';
        return false;
    }

    return true;
}

void closeAcceptor(asio::local::stream_protocol::acceptor& acceptor) {
    std::error_code ignored;
    acceptor.close(ignored);
}

} // namespace

IpcServer::IpcServer(std::string socketPath)
    : socketPath_(std::move(socketPath)),
      acceptor_(io_) {}

IpcServer::~IpcServer() {
    stop();
}

void IpcServer::setHandler(RequestHandler handler) {
    handler_ = std::move(handler);
}

bool IpcServer::start() {
    if (running_) {
        return true;
    }

    if (!handler_) {
        std::cerr << "[ipc] request handler is not set\n";
        return false;
    }

    std::error_code ec;
    const auto parentPath = std::filesystem::path(socketPath_).parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath, ec);
        if (ec) {
            std::cerr << "[ipc] failed to create socket directory: " << ec.message() << '\n';
            return false;
        }
    }

    if (!removeSocketFile(socketPath_)) {
        return false;
    }

    asio::local::stream_protocol::endpoint endpoint(socketPath_);
    io_.restart();

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "[ipc] acceptor open failed: " << ec.message() << '\n';
        return false;
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
        std::cerr << "[ipc] bind failed: " << ec.message() << '\n';
        closeAcceptor(acceptor_);
        return false;
    }

    acceptor_.listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
        std::cerr << "[ipc] listen failed: " << ec.message() << '\n';
        closeAcceptor(acceptor_);
        return false;
    }

    running_ = true;

    asyncAccept();

    ioThread_ = std::thread([this] {
        io_.run();
    });

    std::cout << "[ipc] server started at " << socketPath_ << '\n';
    return true;
}

void IpcServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    std::error_code ec;
    acceptor_.close(ec);
    io_.stop();

    if (ioThread_.joinable()) {
        ioThread_.join();
    }

    removeSocketFile(socketPath_);

    std::cout << "[ipc] server stopped\n";
}

void IpcServer::asyncAccept() {
    acceptor_.async_accept(
        [this](std::error_code ec, asio::local::stream_protocol::socket socket) {
            if (!running_) {
                return;
            }

            if (!ec) {
                auto session = std::make_shared<IpcSession>(
                    std::move(socket),
                    handler_
                );

                session->start();
            } else {
                std::cerr << "[ipc] accept failed: " << ec.message() << '\n';
            }

            asyncAccept();
        }
    );
}

} // namespace iothub::ipc
