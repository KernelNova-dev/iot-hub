// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "IpcProtocol.h"

#include <stdexcept>

namespace wiregate::ipc {

std::array<std::uint8_t, ipcHeaderSize> encodeLength(std::uint32_t value) {
    return {
        static_cast<std::uint8_t>((value >> 24) & 0xFF),
        static_cast<std::uint8_t>((value >> 16) & 0xFF),
        static_cast<std::uint8_t>((value >> 8) & 0xFF),
        static_cast<std::uint8_t>(value & 0xFF)
    };
}

std::uint32_t decodeLength(const std::array<std::uint8_t, ipcHeaderSize>& header) {
    return (static_cast<std::uint32_t>(header[0]) << 24) |
           (static_cast<std::uint32_t>(header[1]) << 16) |
           (static_cast<std::uint32_t>(header[2]) << 8)  |
           (static_cast<std::uint32_t>(header[3]));
}

bool isValidMessageSize(std::uint32_t size) {
    return size > 0 && size <= maxIpcMessageSize;
}

std::vector<std::uint8_t> makeFrame(std::string_view message) {
    if (!isValidMessageSize(static_cast<std::uint32_t>(message.size()))) {
        throw std::runtime_error("Invalid IPC message size");
    }

    const auto header = encodeLength(static_cast<std::uint32_t>(message.size()));

    std::vector<std::uint8_t> frame;
    frame.reserve(header.size() + message.size());
    frame.insert(frame.end(), header.begin(), header.end());
    frame.insert(frame.end(), message.begin(), message.end());

    return frame;
}

} // namespace wiregate::ipc
