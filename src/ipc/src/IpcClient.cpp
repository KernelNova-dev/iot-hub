#include "IpcClient.h"
#include "IpcProtocol.h"

#include <array>
#include <exception>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace iothub::ipc {

IpcClient::IpcClient(std::string socketPath, std::chrono::milliseconds requestTimeout)
    : socketPath_(std::move(socketPath)),
      requestTimeout_(requestTimeout) {}

IpcResponse IpcClient::request(const IpcRequest& request) {
    asio::io_context io;
    asio::local::stream_protocol::socket socket(io);
    asio::steady_timer timer(io);

    auto output = makeFrame(request.serialize());
    std::array<std::uint8_t, ipcHeaderSize> responseHeader{};
    std::vector<char> responseBody;

    std::optional<IpcResponse> response;
    std::exception_ptr failure;
    bool completed = false;

    auto closeSocket = [&socket] {
        std::error_code ignored;
        socket.close(ignored);
    };

    auto completeWithError = [&](std::exception_ptr error) {
        if (completed) {
            return;
        }

        completed = true;
        failure = std::move(error);
        closeSocket();
        timer.cancel();
    };

    auto completeWithSystemError = [&](const std::error_code& ec, std::string operation) {
        completeWithError(std::make_exception_ptr(std::system_error(ec, std::move(operation))));
    };

    auto completeWithResponse = [&](IpcResponse value) {
        if (completed) {
            return;
        }

        completed = true;
        response = std::move(value);
        closeSocket();
        timer.cancel();
    };

    timer.expires_after(requestTimeout_);
    timer.async_wait([&](const std::error_code& ec) {
        if (!ec) {
            completeWithError(std::make_exception_ptr(std::runtime_error("IPC request timed out")));
        }
    });

    socket.async_connect(
        asio::local::stream_protocol::endpoint(socketPath_),
        [&](const std::error_code& ec) {
            if (completed) {
                return;
            }

            if (ec) {
                completeWithSystemError(ec, "IPC connect failed");
                return;
            }

            asio::async_write(
                socket,
                asio::buffer(output),
                [&](const std::error_code& ec, std::size_t) {
                    if (completed) {
                        return;
                    }

                    if (ec) {
                        completeWithSystemError(ec, "IPC write failed");
                        return;
                    }

                    asio::async_read(
                        socket,
                        asio::buffer(responseHeader),
                        [&](const std::error_code& ec, std::size_t) {
                            if (completed) {
                                return;
                            }

                            if (ec) {
                                completeWithSystemError(ec, "IPC response header read failed");
                                return;
                            }

                            const auto responseSize = decodeLength(responseHeader);
                            if (!isValidMessageSize(responseSize)) {
                                completeWithError(std::make_exception_ptr(
                                    std::runtime_error("Invalid IPC response size")
                                ));
                                return;
                            }

                            responseBody.resize(responseSize);
                            asio::async_read(
                                socket,
                                asio::buffer(responseBody),
                                [&](const std::error_code& ec, std::size_t) {
                                    if (completed) {
                                        return;
                                    }

                                    if (ec) {
                                        completeWithSystemError(ec, "IPC response body read failed");
                                        return;
                                    }

                                    try {
                                        completeWithResponse(IpcResponse::deserialize(
                                            std::string(responseBody.begin(), responseBody.end())
                                        ));
                                    } catch (...) {
                                        completeWithError(std::current_exception());
                                    }
                                }
                            );
                        }
                    );
                }
            );
        }
    );

    io.run();

    if (failure) {
        std::rethrow_exception(failure);
    }

    if (!response) {
        throw std::runtime_error("IPC request did not complete");
    }

    return std::move(*response);
}

} // namespace iothub::ipc
