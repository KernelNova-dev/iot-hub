// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Service.h"

#include <cstdint>

// ############################################################

namespace wiregate {

class DevicePoolModule : public ServiceModule {
public:
    DevicePoolModule();
    ~DevicePoolModule() override;

    void initialize(const ServiceSettings& settings) override;
    void start(const ServiceContext&) override;
    void execute(const ServiceContext&) override;
    void shutdown() override;

private:
    std::uint64_t tick{0};
};

} // namespace wiregate
