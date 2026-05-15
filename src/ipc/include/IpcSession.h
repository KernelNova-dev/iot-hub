#pragma once

#include "IpcMessage.h"
#include "IpcProtocol.h"

#include <asio.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace iothub::ipc {

class IpcSession : public std::enable_shared_from_this<IpcSession> {
public:
    using Socket = asio::local::stream_protocol::socket;
    using RequestHandler = std::function<IpcResponse(const IpcRequest& request)>;

    IpcSession(Socket socket, RequestHandler handler);

    void start();

private:
    void readHeader();
    void readBody(std::uint32_t bodySize);
    void writeResponse(const IpcResponse& response);

private:
    Socket socket_;
    RequestHandler handler_;

    std::array<std::uint8_t, ipcHeaderSize> header_{};
    std::vector<std::uint8_t> body_;
    std::vector<std::uint8_t> writeBuffer_;
};

} // namespace iothub::ipc
