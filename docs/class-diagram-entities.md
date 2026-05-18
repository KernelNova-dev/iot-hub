<!--
SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# WireGate Generic Entity Relations

This diagram focuses on runtime relationships between the main service, modules, IPC infrastructure, and device model.

![WireGate generic entity relations](./class-diagram-entities.svg)

```mermaid
classDiagram
    direction LR

    class WireGateService {
        owns service lifecycle
        owns module list
        creates ServiceContext
    }

    class ServiceContext {
        ServiceSettings*
        DeviceManager
    }

    class ServiceSettings {
        ipcSocketPath
        serviceIdleTimeout
    }

    class ServiceModule {
        initialize(settings)
        start(context)
        execute(context)
        shutdown()
    }

    class IpcServiceModule {
        handles IPC requests
    }

    class DevicePoolModule {
        manages demo device data
    }

    class DeviceManager {
        list()
        find(id)
        sendCommand(id, command)
    }

    class DeviceRegistry {
        stores device state
    }

    class DeviceInfo {
        id
        name
        type
        status
        telemetry
        lastCommand
    }

    class IpcServer {
        listens on local socket
        dispatches requests
    }

    class IpcRequest {
        type
        deviceId
        command
    }

    class IpcResponse {
        ok
        payload
        message
    }

    WireGateService --> ServiceSettings : uses
    WireGateService --> ServiceContext : creates
    WireGateService o-- ServiceModule : owns
    ServiceContext --> DeviceManager : exposes

    ServiceModule <|-- IpcServiceModule
    ServiceModule <|-- DevicePoolModule

    DeviceManager <|-- DeviceRegistry
    DeviceRegistry o-- DeviceInfo : stores

    IpcServiceModule --> IpcServer : owns
    IpcServiceModule --> DeviceManager : queries/commands
    IpcServer --> IpcRequest : receives
    IpcServer --> IpcResponse : returns
    DevicePoolModule --> DeviceManager : updates
```
