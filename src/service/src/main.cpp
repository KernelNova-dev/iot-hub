#include "Service.h"

#include <atomic>
#include <csignal>

namespace {

std::atomic_bool stopRequested{false};

void handleSignal(int) {
    stopRequested = true;
}

} // namespace

int main() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    iothub::service::Service service;

    if (!service.start()) {
        return 1;
    }

    service.run(stopRequested);
    return 0;
}
