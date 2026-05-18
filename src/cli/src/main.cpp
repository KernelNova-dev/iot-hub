// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "IpcClient.h"
#include "IpcMessage.h"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cctype>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

using wiregate::ipc::IpcClient;
using wiregate::ipc::IpcRequest;
using wiregate::ipc::IpcResponse;

constexpr const char* defaultSocketPath = "/tmp/wiregate-service.sock";
constexpr int requestFailedExitCode = 2;

enum class CliAction {
    none,
    status,
    list,
    get,
    sendCommand,
    health,
    telemetry,
    commands,
    watch
};

struct Cell {
    std::string text;
    std::string rendered;
};

using Row = std::vector<Cell>;

struct ListFilter {
    std::string type;
    std::string status;
    std::string location;
};

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool containsInsensitive(std::string value, std::string pattern) {
    return toLower(std::move(value)).find(toLower(std::move(pattern))) != std::string::npos;
}

std::string colorize(std::string_view text, std::string_view code, bool useColor) {
    if (!useColor) {
        return std::string{text};
    }

    std::string result;
    result.reserve(text.size() + code.size() + 8);
    result += "\033[";
    result += code;
    result += 'm';
    result += text;
    result += "\033[0m";
    return result;
}

Cell makeCell(std::string text) {
    return Cell{
        .text = std::move(text),
        .rendered = {}
    };
}

Cell makeCell(std::string text, std::string_view colorCode, bool useColor) {
    return Cell{
        .text = text,
        .rendered = colorize(text, colorCode, useColor)
    };
}

std::string renderCell(const Cell& cell) {
    if (!cell.rendered.empty()) {
        return cell.rendered;
    }

    return cell.text;
}

void printBorder(const std::vector<std::size_t>& widths, std::string_view left, std::string_view join, std::string_view right) {
    std::cout << left;
    for (std::size_t i = 0; i < widths.size(); ++i) {
        std::cout << std::string(widths[i] + 2, '-');
        std::cout << (i + 1 == widths.size() ? right : join);
    }
    std::cout << '\n';
}

void printRow(const Row& row, const std::vector<std::size_t>& widths) {
    std::cout << '|';
    for (std::size_t i = 0; i < widths.size(); ++i) {
        const auto& cell = row[i];
        std::cout << ' ' << renderCell(cell) << std::string(widths[i] - cell.text.size(), ' ') << " |";
    }
    std::cout << '\n';
}

void printTable(const Row& header, const std::vector<Row>& rows, bool useColor) {
    std::vector<std::size_t> widths(header.size(), 0);

    for (std::size_t i = 0; i < header.size(); ++i) {
        widths[i] = header[i].text.size();
    }

    for (const auto& row : rows) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            widths[i] = std::max(widths[i], row[i].text.size());
        }
    }

    Row renderedHeader;
    renderedHeader.reserve(header.size());
    for (const auto& cell : header) {
        renderedHeader.push_back(makeCell(cell.text, "1;37", useColor));
    }

    printBorder(widths, "+", "+", "+");
    printRow(renderedHeader, widths);
    printBorder(widths, "+", "+", "+");
    for (const auto& row : rows) {
        printRow(row, widths);
    }
    printBorder(widths, "+", "+", "+");
}

std::string jsonValueToString(const nlohmann::json& value) {
    if (value.is_null()) {
        return "-";
    }

    if (value.is_string()) {
        auto text = value.get<std::string>();
        return text.empty() ? "-" : text;
    }

    if (value.is_boolean()) {
        return value.get<bool>() ? "true" : "false";
    }

    if (value.is_number_float()) {
        std::ostringstream output;
        output << value.get<double>();
        return output.str();
    }

    if (value.is_number_integer()) {
        return std::to_string(value.get<long long>());
    }

    if (value.is_number_unsigned()) {
        return std::to_string(value.get<unsigned long long>());
    }

    return value.dump();
}

std::string objectToString(const nlohmann::json& object) {
    if (!object.is_object() || object.empty()) {
        return "-";
    }

    std::string result;
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!result.empty()) {
            result += ", ";
        }
        result += it.key();
        result += '=';
        result += jsonValueToString(it.value());
    }

    return result;
}

std::string jsonArrayToString(const nlohmann::json& array) {
    if (!array.is_array() || array.empty()) {
        return "-";
    }

    std::string result;
    for (const auto& item : array) {
        if (!result.empty()) {
            result += ", ";
        }
        result += jsonValueToString(item);
    }

    return result;
}

Cell statusCell(const std::string& status, bool useColor) {
    if (status == "online" || status == "running" || status == "on") {
        return makeCell(status, "32", useColor);
    }

    if (status == "offline" || status == "stopped" || status == "error" || status == "warning") {
        return makeCell(status, "31", useColor);
    }

    if (status == "off") {
        return makeCell(status, "33", useColor);
    }

    return makeCell(status, "36", useColor);
}

void printTitle(std::string_view title, bool useColor) {
    std::cout << colorize(title, "1;36", useColor) << '\n';
}

void printError(std::string_view message, bool useColor) {
    std::cerr << colorize("✕ Error: ", "1;31", useColor) << message << '\n';
}

IpcRequest makeRequest(std::string type, std::string deviceId = {}, std::string command = {}) {
    return IpcRequest{
        .type = std::move(type),
        .deviceId = std::move(deviceId),
        .command = std::move(command)
    };
}

bool matchesFilter(const nlohmann::json& device, const ListFilter& filter) {
    if (!filter.type.empty() && device.value("type", "") != filter.type) {
        return false;
    }

    if (!filter.status.empty() && device.value("status", "") != filter.status) {
        return false;
    }

    if (!filter.location.empty() && !containsInsensitive(device.value("location", ""), filter.location)) {
        return false;
    }

    return true;
}

std::vector<nlohmann::json> devicesFromPayload(const nlohmann::json& payload, const ListFilter& filter = {}) {
    std::vector<nlohmann::json> devices;

    for (const auto& device : payload.value("devices", nlohmann::json::array())) {
        if (matchesFilter(device, filter)) {
            devices.push_back(device);
        }
    }

    return devices;
}

Row deviceRow(const nlohmann::json& device, bool useColor) {
    const auto status = device.value("status", "-");

    return Row{
        makeCell(device.value("id", "-")),
        makeCell(device.value("name", "-")),
        makeCell(device.value("type", "-")),
        makeCell(device.value("location", "-")),
        statusCell(status.empty() ? "-" : status, useColor),
        makeCell(objectToString(device.value("telemetry", nlohmann::json::object()))),
        makeCell(jsonValueToString(device.value("lastCommand", nlohmann::json{})))
    };
}

void printStatus(const nlohmann::json& payload, bool useColor) {
    const bool running = payload.value("running", false);
    const auto deviceCount = payload.value("deviceCount", 0);

    printTitle("● WireGate service", useColor);
    printTable(
        Row{makeCell("Property"), makeCell("Value")},
        std::vector<Row>{
            Row{makeCell("State"), statusCell(running ? "running" : "stopped", useColor)},
            Row{makeCell("Devices"), makeCell(std::to_string(deviceCount))}
        },
        useColor
    );
}

void printDeviceTable(const std::vector<nlohmann::json>& devices, bool useColor) {
    printTitle("◆ Devices", useColor);

    if (devices.empty()) {
        std::cout << colorize("No devices matched.", "33", useColor) << '\n';
        return;
    }

    std::vector<Row> rows;
    rows.reserve(devices.size());
    for (const auto& device : devices) {
        rows.push_back(deviceRow(device, useColor));
    }

    printTable(
        Row{
            makeCell("ID"),
            makeCell("Name"),
            makeCell("Type"),
            makeCell("Location"),
            makeCell("Status"),
            makeCell("Telemetry"),
            makeCell("Last command")
        },
        rows,
        useColor
    );
}

void printDevice(const nlohmann::json& payload, bool useColor) {
    printDeviceTable(std::vector<nlohmann::json>{payload.value("device", nlohmann::json::object())}, useColor);
}

void printDevices(const nlohmann::json& payload, const ListFilter& filter, bool useColor) {
    printDeviceTable(devicesFromPayload(payload, filter), useColor);
}

void printCommandResult(const nlohmann::json& payload, bool useColor) {
    std::cout << colorize("✓ Command accepted", "1;32", useColor) << '\n';
    printTable(
        Row{makeCell("Property"), makeCell("Value")},
        std::vector<Row>{
            Row{makeCell("Device"), makeCell(payload.value("deviceId", "-"))},
            Row{makeCell("Command"), makeCell(payload.value("command", "-"))}
        },
        useColor
    );
}

void printHealth(const nlohmann::json& statusPayload, const std::vector<nlohmann::json>& devices, bool useColor) {
    std::map<std::string, int> statusCounts;
    std::map<std::string, int> typeCounts;
    std::set<std::string> locations;

    int attentionCount = 0;
    for (const auto& device : devices) {
        const auto status = device.value("status", "unknown");
        ++statusCounts[status];
        ++typeCounts[device.value("type", "unknown")];
        locations.insert(device.value("location", "unknown"));

        if (status == "offline" || status == "error" || status == "warning") {
            ++attentionCount;
        }
    }

    printTitle("◆ Health", useColor);
    printTable(
        Row{makeCell("Property"), makeCell("Value")},
        std::vector<Row>{
            Row{makeCell("Service"), statusCell(statusPayload.value("running", false) ? "running" : "stopped", useColor)},
            Row{makeCell("Devices"), makeCell(std::to_string(devices.size()))},
            Row{makeCell("Locations"), makeCell(std::to_string(locations.size()))},
            Row{makeCell("Attention"), makeCell(std::to_string(attentionCount), attentionCount == 0 ? "32" : "31", useColor)}
        },
        useColor
    );

    std::vector<Row> statusRows;
    for (const auto& [status, count] : statusCounts) {
        statusRows.push_back(Row{statusCell(status, useColor), makeCell(std::to_string(count))});
    }

    std::cout << '\n';
    printTitle("Status breakdown", useColor);
    printTable(Row{makeCell("Status"), makeCell("Count")}, statusRows, useColor);

    std::vector<Row> typeRows;
    for (const auto& [type, count] : typeCounts) {
        typeRows.push_back(Row{makeCell(type), makeCell(std::to_string(count))});
    }

    std::cout << '\n';
    printTitle("Device types", useColor);
    printTable(Row{makeCell("Type"), makeCell("Count")}, typeRows, useColor);
}

void printTelemetry(const nlohmann::json& device, bool useColor) {
    printTitle("◆ Telemetry: " + device.value("name", device.value("id", "-")), useColor);
    printTable(
        Row{makeCell("Property"), makeCell("Value")},
        std::vector<Row>{
            Row{makeCell("ID"), makeCell(device.value("id", "-"))},
            Row{makeCell("Type"), makeCell(device.value("type", "-"))},
            Row{makeCell("Location"), makeCell(device.value("location", "-"))},
            Row{makeCell("Status"), statusCell(device.value("status", "-"), useColor)}
        },
        useColor
    );

    const auto telemetry = device.value("telemetry", nlohmann::json::object());
    std::vector<Row> telemetryRows;
    for (auto it = telemetry.begin(); it != telemetry.end(); ++it) {
        telemetryRows.push_back(Row{makeCell(it.key()), makeCell(jsonValueToString(it.value()))});
    }

    std::cout << '\n';
    printTitle("Measurements", useColor);
    printTable(Row{makeCell("Metric"), makeCell("Value")}, telemetryRows, useColor);

    const auto settings = device.value("settings", nlohmann::json::object());
    std::vector<Row> settingRows;
    for (auto it = settings.begin(); it != settings.end(); ++it) {
        settingRows.push_back(Row{makeCell(it.key()), makeCell(jsonValueToString(it.value()))});
    }

    if (!settingRows.empty()) {
        std::cout << '\n';
        printTitle("Settings", useColor);
        printTable(Row{makeCell("Setting"), makeCell("Value")}, settingRows, useColor);
    }
}

void printSupportedCommands(const std::vector<nlohmann::json>& devices, bool useColor) {
    printTitle("◆ Supported commands", useColor);

    std::vector<Row> rows;
    rows.reserve(devices.size());
    for (const auto& device : devices) {
        rows.push_back(Row{
            makeCell(device.value("id", "-")),
            makeCell(device.value("type", "-")),
            makeCell(device.value("location", "-")),
            makeCell(jsonArrayToString(device.value("supportedCommands", nlohmann::json::array())))
        });
    }

    printTable(
        Row{makeCell("Device"), makeCell("Type"), makeCell("Location"), makeCell("Commands")},
        rows,
        useColor
    );
}

bool shouldUseColor(const std::string& mode) {
    if (mode == "always") {
        return true;
    }

    if (mode == "never" || std::getenv("NO_COLOR") != nullptr) {
        return false;
    }

    return isatty(STDOUT_FILENO) != 0;
}

IpcResponse requestDevices(IpcClient& client) {
    return client.request(makeRequest("list-devices"));
}

IpcResponse requestStatus(IpcClient& client) {
    return client.request(makeRequest("service-status"));
}

IpcResponse requestDevice(IpcClient& client, const std::string& deviceId) {
    return client.request(makeRequest("get-device", deviceId));
}

int handleIpcError(const IpcResponse& response, bool useColor) {
    if (response.ok) {
        return 0;
    }

    printError(response.message, useColor);
    return requestFailedExitCode;
}

int runStatus(IpcClient& client, bool useColor) {
    const auto response = requestStatus(client);
    if (!response.ok) {
        return handleIpcError(response, useColor);
    }

    printStatus(response.payload, useColor);
    return 0;
}

int runList(IpcClient& client, const ListFilter& filter, bool useColor) {
    const auto response = requestDevices(client);
    if (!response.ok) {
        return handleIpcError(response, useColor);
    }

    printDevices(response.payload, filter, useColor);
    return 0;
}

int runGet(IpcClient& client, const std::string& deviceId, bool useColor) {
    const auto response = requestDevice(client, deviceId);
    if (!response.ok) {
        return handleIpcError(response, useColor);
    }

    printDevice(response.payload, useColor);
    return 0;
}

int runSendCommand(IpcClient& client, const std::string& deviceId, const std::string& command, bool useColor) {
    const auto response = client.request(makeRequest("send-command", deviceId, command));
    if (!response.ok) {
        return handleIpcError(response, useColor);
    }

    printCommandResult(response.payload, useColor);
    return 0;
}

int runHealth(IpcClient& client, bool useColor) {
    const auto statusResponse = requestStatus(client);
    if (!statusResponse.ok) {
        return handleIpcError(statusResponse, useColor);
    }

    const auto devicesResponse = requestDevices(client);
    if (!devicesResponse.ok) {
        return handleIpcError(devicesResponse, useColor);
    }

    printHealth(statusResponse.payload, devicesFromPayload(devicesResponse.payload), useColor);
    return 0;
}

int runTelemetry(IpcClient& client, const std::string& deviceId, bool useColor) {
    const auto response = requestDevice(client, deviceId);
    if (!response.ok) {
        return handleIpcError(response, useColor);
    }

    printTelemetry(response.payload.value("device", nlohmann::json::object()), useColor);
    return 0;
}

int runCommands(IpcClient& client, const std::string& deviceId, bool useColor) {
    if (!deviceId.empty()) {
        const auto response = requestDevice(client, deviceId);
        if (!response.ok) {
            return handleIpcError(response, useColor);
        }

        printSupportedCommands(std::vector<nlohmann::json>{response.payload.value("device", nlohmann::json::object())}, useColor);
        return 0;
    }

    const auto response = requestDevices(client);
    if (!response.ok) {
        return handleIpcError(response, useColor);
    }

    printSupportedCommands(devicesFromPayload(response.payload), useColor);
    return 0;
}

std::string currentTimeText() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);

    std::ostringstream output;
    output << std::put_time(std::localtime(&time), "%H:%M:%S");
    return output.str();
}

void clearScreen() {
    if (isatty(STDOUT_FILENO) != 0) {
        std::cout << "\033[2J\033[H";
    }
}

int runWatch(IpcClient& client, const std::string& view, const ListFilter& filter, int intervalSec, bool useColor) {
    while (true) {
        clearScreen();
        std::cout << colorize("↻ Watch: " + view + " | " + currentTimeText(), "1;36", useColor) << "\n\n";

        if (view == "status") {
            runStatus(client, useColor);
        } else if (view == "health") {
            runHealth(client, useColor);
        } else {
            runList(client, filter, useColor);
        }

        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::seconds{intervalSec});
    }
}

} // namespace

int main(int argc, char* argv[]) {
    CLI::App app{"WireGate command-line client"};

    std::string socketPath = defaultSocketPath;
    std::string colorMode = "auto";
    int timeoutMs = 5000;
    int watchIntervalSec = 2;
    std::string watchView = "list";
    std::string deviceId;
    std::string command;
    ListFilter listFilter;
    ListFilter watchFilter;
    CliAction action = CliAction::none;

    app.add_option("--socket", socketPath, "IPC socket path")
        ->default_val(defaultSocketPath);
    app.add_option("--timeout", timeoutMs, "IPC request timeout in milliseconds")
        ->default_val(timeoutMs)
        ->check(CLI::PositiveNumber);
    app.add_option("--color", colorMode, "Color mode: auto, always, never")
        ->default_val(colorMode)
        ->check(CLI::IsMember({"auto", "always", "never"}));
    app.require_subcommand(1);

    auto status = app.add_subcommand("status", "Request service status");
    status->callback([&] {
        action = CliAction::status;
    });

    auto list = app.add_subcommand("list", "List known devices");
    list->add_option("--type", listFilter.type, "Filter by device type");
    list->add_option("--status", listFilter.status, "Filter by device status");
    list->add_option("--location", listFilter.location, "Filter by location text");
    list->callback([&] {
        action = CliAction::list;
    });

    auto get = app.add_subcommand("get", "Get one device by id");
    get->add_option("device-id", deviceId, "Device id")
        ->required();
    get->callback([&] {
        action = CliAction::get;
    });

    auto send = app.add_subcommand("command", "Send a command to a device");
    send->add_option("device-id", deviceId, "Device id")
        ->required();
    send->add_option("command", command, "Command name")
        ->required();
    send->callback([&] {
        action = CliAction::sendCommand;
    });

    auto health = app.add_subcommand("health", "Show service and device health summary");
    health->callback([&] {
        action = CliAction::health;
    });

    auto telemetry = app.add_subcommand("telemetry", "Show telemetry and settings for one device");
    telemetry->add_option("device-id", deviceId, "Device id")
        ->required();
    telemetry->callback([&] {
        action = CliAction::telemetry;
    });

    auto commands = app.add_subcommand("commands", "Show supported device commands");
    commands->add_option("device-id", deviceId, "Optional device id");
    commands->callback([&] {
        action = CliAction::commands;
    });

    auto watch = app.add_subcommand("watch", "Redraw a service view periodically");
    watch->add_option("view", watchView, "View to watch: status, health, list")
        ->default_val(watchView)
        ->check(CLI::IsMember({"status", "health", "list"}));
    watch->add_option("--interval", watchIntervalSec, "Refresh interval in seconds")
        ->default_val(watchIntervalSec)
        ->check(CLI::PositiveNumber);
    watch->add_option("--type", watchFilter.type, "Filter list view by device type");
    watch->add_option("--status", watchFilter.status, "Filter list view by device status");
    watch->add_option("--location", watchFilter.location, "Filter list view by location text");
    watch->callback([&] {
        action = CliAction::watch;
    });

    CLI11_PARSE(app, argc, argv);

    if (action == CliAction::none) {
        std::cerr << "error: no command selected\n";
        return 1;
    }

    try {
        const bool useColor = shouldUseColor(colorMode);
        IpcClient client{socketPath, std::chrono::milliseconds{timeoutMs}};

        switch (action) {
            case CliAction::status:
                return runStatus(client, useColor);
            case CliAction::list:
                return runList(client, listFilter, useColor);
            case CliAction::get:
                return runGet(client, deviceId, useColor);
            case CliAction::sendCommand:
                return runSendCommand(client, deviceId, command, useColor);
            case CliAction::health:
                return runHealth(client, useColor);
            case CliAction::telemetry:
                return runTelemetry(client, deviceId, useColor);
            case CliAction::commands:
                return runCommands(client, deviceId, useColor);
            case CliAction::watch:
                return runWatch(client, watchView, watchFilter, watchIntervalSec, useColor);
            case CliAction::none:
                break;
        }
    } catch (const std::exception& ex) {
        const bool useColor = shouldUseColor(colorMode);
        printError(ex.what(), useColor);
        return 1;
    }

    return 1;
}
