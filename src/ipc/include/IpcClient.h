// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "IpcMessage.h"

#include <asio.hpp>

#include <chrono>
#include <string>

namespace wiregate::ipc {

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

} // namespace wiregate::ipc
