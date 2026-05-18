// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "IpcClient.h"
#include "IpcMessage.h"

#include <crow.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>

namespace {

using wiregate::ipc::IpcClient;
using wiregate::ipc::IpcRequest;
using wiregate::ipc::IpcResponse;

struct WebSettings {
    std::string appTitle{"WireGate Control"};
    std::string bindAddress{"127.0.0.1"};
    std::uint16_t port{8080};
    std::string ipcSocketPath{"/tmp/wiregate-service.sock"};
    std::filesystem::path staticRoot{"web"};
};

std::filesystem::path parentPath(const std::filesystem::path& path) {
    const auto parent = path.parent_path();
    return parent.empty() ? std::filesystem::current_path() : parent;
}

std::filesystem::path resolveStaticRoot(const std::filesystem::path& configPath, const std::filesystem::path& configuredRoot) {
    if (configuredRoot.is_absolute() && std::filesystem::exists(configuredRoot)) {
        return configuredRoot;
    }

    const auto configDir = parentPath(configPath);
    const auto workspaceRoot = configDir.parent_path();

    const std::filesystem::path candidates[] = {
        std::filesystem::current_path() / configuredRoot,
        configDir / configuredRoot,
        workspaceRoot / configuredRoot,
        workspaceRoot / "src" / "gui" / configuredRoot
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return std::filesystem::weakly_canonical(candidate);
        }
    }

    return configuredRoot;
}

std::filesystem::path resolveConfigPath(int argc, char* argv[]) {
    if (argc > 1) {
        return argv[1];
    }

    if (const char* envPath = std::getenv("WIREGATE_WEB_CONFIG")) {
        return envPath;
    }

    return "config/wiregate-web.json";
}

WebSettings loadWebSettings(const std::filesystem::path& configPath) {
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open web config file: " + configPath.string());
    }

    nlohmann::json config;
    configFile >> config;

    WebSettings settings;
    settings.appTitle = config.value("appTitle", settings.appTitle);
    settings.bindAddress = config.value("bindAddress", settings.bindAddress);
    settings.port = static_cast<std::uint16_t>(config.value("port", settings.port));
    settings.ipcSocketPath = config.value("ipcSocketPath", settings.ipcSocketPath);
    settings.staticRoot = resolveStaticRoot(configPath, config.value("staticRoot", settings.staticRoot.string()));

    return settings;
}

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path.string());
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string contentTypeFor(const std::filesystem::path& path) {
    const auto extension = path.extension().string();

    if (extension == ".html") {
        return "text/html; charset=utf-8";
    }
    if (extension == ".css") {
        return "text/css; charset=utf-8";
    }
    if (extension == ".js") {
        return "application/javascript; charset=utf-8";
    }
    if (extension == ".svg") {
        return "image/svg+xml";
    }

    return "text/plain; charset=utf-8";
}

crow::response textResponse(std::string body, std::string contentType = "text/plain; charset=utf-8", int status = 200) {
    crow::response response{status};
    response.set_header("Content-Type", contentType);
    response.body = std::move(body);
    return response;
}

crow::response jsonResponse(const nlohmann::json& body, int status = 200) {
    crow::response response{status};
    response.set_header("Content-Type", "application/json; charset=utf-8");
    response.body = body.dump();
    return response;
}

crow::response fileResponse(const WebSettings& settings, const std::filesystem::path& relativePath) {
    const auto fullPath = std::filesystem::weakly_canonical(settings.staticRoot / relativePath);
    const auto root = std::filesystem::weakly_canonical(settings.staticRoot);

    if (fullPath.string().find(root.string()) != 0 || !std::filesystem::is_regular_file(fullPath)) {
        return textResponse("Not found", "text/plain; charset=utf-8", 404);
    }

    try {
        return textResponse(readTextFile(fullPath), contentTypeFor(fullPath));
    } catch (const std::exception& ex) {
        return jsonResponse({{"error", ex.what()}}, 500);
    }
}

IpcResponse sendIpcRequest(const WebSettings& settings, IpcRequest request) {
    IpcClient client{settings.ipcSocketPath, std::chrono::seconds{3}};
    return client.request(request);
}

crow::response ipcResponseToHttp(const IpcResponse& ipcResponse, int successStatus = 200) {
    if (!ipcResponse.ok) {
        return jsonResponse({{"error", ipcResponse.message}}, 400);
    }

    return jsonResponse(ipcResponse.payload, successStatus);
}

IpcRequest makeIpcRequest(std::string type, std::string deviceId = {}, std::string command = {}) {
    return IpcRequest{
        .type = std::move(type),
        .deviceId = std::move(deviceId),
        .command = std::move(command)
    };
}

nlohmann::json buildHealthPayload(const nlohmann::json& statusPayload, const nlohmann::json& devicesPayload) {
    std::map<std::string, int> statusCounts;
    std::map<std::string, int> typeCounts;
    int attentionCount = 0;

    const auto devices = devicesPayload.value("devices", nlohmann::json::array());
    for (const auto& device : devices) {
        const auto status = device.value("status", "unknown");
        ++statusCounts[status];
        ++typeCounts[device.value("type", "unknown")];

        if (status == "offline" || status == "error" || status == "warning") {
            ++attentionCount;
        }
    }

    return {
        {"service", statusPayload},
        {"deviceCount", devices.size()},
        {"attentionCount", attentionCount},
        {"statusCounts", statusCounts},
        {"typeCounts", typeCounts}
    };
}

crow::response handleIpcRequest(const WebSettings& settings, IpcRequest request) {
    try {
        return ipcResponseToHttp(sendIpcRequest(settings, std::move(request)));
    } catch (const std::exception& ex) {
        return jsonResponse({{"error", ex.what()}}, 502);
    }
}

crow::response handleHealthRequest(const WebSettings& settings) {
    try {
        const auto status = sendIpcRequest(settings, makeIpcRequest("service-status"));
        if (!status.ok) {
            return ipcResponseToHttp(status);
        }

        const auto devices = sendIpcRequest(settings, makeIpcRequest("list-devices"));
        if (!devices.ok) {
            return ipcResponseToHttp(devices);
        }

        return jsonResponse(buildHealthPayload(status.payload, devices.payload));
    } catch (const std::exception& ex) {
        return jsonResponse({{"error", ex.what()}}, 502);
    }
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const auto configPath = resolveConfigPath(argc, argv);
        const auto settings = loadWebSettings(configPath);

        crow::SimpleApp app;
        app.loglevel(crow::LogLevel::Warning);

        CROW_ROUTE(app, "/")([settings] {
            return fileResponse(settings, "index.html");
        });

        CROW_ROUTE(app, "/assets/<path>")([settings](const std::string& path) {
            return fileResponse(settings, std::filesystem::path{"assets"} / path);
        });

        CROW_ROUTE(app, "/api/status")([settings] {
            return handleIpcRequest(settings, makeIpcRequest("service-status"));
        });

        CROW_ROUTE(app, "/api/health")([settings] {
            return handleHealthRequest(settings);
        });

        CROW_ROUTE(app, "/api/devices")([settings] {
            return handleIpcRequest(settings, makeIpcRequest("list-devices"));
        });

        CROW_ROUTE(app, "/api/devices/<string>")([settings](const std::string& deviceId) {
            return handleIpcRequest(settings, makeIpcRequest("get-device", deviceId));
        });

        CROW_ROUTE(app, "/api/devices/<string>/commands")
            .methods(crow::HTTPMethod::POST)([settings](const crow::request& request, const std::string& deviceId) {
                try {
                    const auto body = nlohmann::json::parse(request.body);
                    const auto command = body.value("command", "");

                    if (command.empty()) {
                        return jsonResponse({{"error", "command is required"}}, 400);
                    }

                    return handleIpcRequest(settings, makeIpcRequest("send-command", deviceId, command));
                } catch (const std::exception& ex) {
                    return jsonResponse({{"error", ex.what()}}, 400);
                }
            });

        std::cout << "WireGate web UI: http://" << settings.bindAddress << ':' << settings.port << '\n';
        std::cout << "Static root: " << settings.staticRoot << '\n';

        app.bindaddr(settings.bindAddress)
            .port(settings.port)
            .multithreaded()
            .run();
    } catch (const std::exception& ex) {
        std::cerr << "wiregate-gui: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
