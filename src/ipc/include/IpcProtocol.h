// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace wiregate::ipc {

inline constexpr std::uint32_t maxIpcMessageSize = 1024 * 1024;
inline constexpr std::size_t ipcHeaderSize = 4;

[[nodiscard]] std::array<std::uint8_t, ipcHeaderSize> encodeLength(std::uint32_t value);
[[nodiscard]] std::uint32_t decodeLength(const std::array<std::uint8_t, ipcHeaderSize>& header);
[[nodiscard]] bool isValidMessageSize(std::uint32_t size);
[[nodiscard]] std::vector<std::uint8_t> makeFrame(std::string_view message);

} // namespace wiregate::ipc
