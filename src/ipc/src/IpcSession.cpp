// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "IpcSession.h"

#include <iostream>
#include <stdexcept>

namespace wiregate::ipc {

IpcSession::IpcSession(Socket socket, RequestHandler handler)
    : socket_(std::move(socket)),
      handler_(std::move(handler)) {}

void IpcSession::start() {
    readHeader();
}

void IpcSession::readHeader() {
    auto self = shared_from_this();

    asio::async_read(
        socket_,
        asio::buffer(header_),
        [this, self](std::error_code ec, std::size_t) {
            if (ec) {
                return;
            }

            const auto bodySize = decodeLength(header_);

            if (!isValidMessageSize(bodySize)) {
                std::cerr << "[ipc] invalid message size\n";
                return;
            }

            readBody(bodySize);
        }
    );
}

void IpcSession::readBody(std::uint32_t bodySize) {
    auto self = shared_from_this();

    body_.resize(bodySize);

    asio::async_read(
        socket_,
        asio::buffer(body_),
        [this, self](std::error_code ec, std::size_t) {
            if (ec) {
                return;
            }

            try {
                const auto request = IpcRequest::deserialize(std::string(body_.begin(), body_.end()));
                writeResponse(handler_(request));
            } catch (const std::exception& ex) {
                writeResponse(IpcResponse::error(ex.what()));
            } catch (...) {
                writeResponse(IpcResponse::error("Unknown IPC handler error"));
            }
        }
    );
}

void IpcSession::writeResponse(const IpcResponse& response) {
    writeBuffer_ = makeFrame(response.serialize());

    auto self = shared_from_this();

    asio::async_write(
        socket_,
        asio::buffer(writeBuffer_),
        [this, self](std::error_code, std::size_t) {
            std::error_code ignored;
            socket_.shutdown(asio::socket_base::shutdown_both, ignored);
            socket_.close(ignored);
        }
    );
}

} // namespace wiregate::ipc
