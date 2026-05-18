<!--
SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# WireGate Concept Class Diagram

This is the compact class diagram for explaining the general design. It avoids low-value members and focuses on the core idea: a service orchestrates modules, modules use shared context, and device state is accessed through an abstraction.

![WireGate concept class diagram](./class-diagram-concept.svg)

```mermaid
classDiagram
    direction LR

    class WireGateService {
        +installModule(module)
        +startMainLoop(settings)
        +shutdown()
        -mainLoop()
    }

    class ServiceModule {
        <<interface>>
        +initialize(settings)
        +start(context)
        +execute(context)
        +shutdown()
    }

    class ServiceContext {
        +settings
        +deviceManager
    }

    class DeviceManager {
        <<interface>>
        +list()
        +find(id)
        +sendCommand(id, command)
    }

    class DeviceRegistry {
        +updateOrInsert(device)
        +updateTelemetry(id, telemetry)
        +setStatus(id, status)
    }

    class IpcServiceModule {
        +handleRequest(request)
    }

    class DevicePoolModule {
        +execute(context)
    }

    class IpcServer {
        +start()
        +stop()
        +setHandler(handler)
    }

    WireGateService o-- ServiceModule
    WireGateService --> ServiceContext
    ServiceContext --> DeviceManager

    ServiceModule <|-- IpcServiceModule
    ServiceModule <|-- DevicePoolModule

    DeviceManager <|-- DeviceRegistry
    IpcServiceModule --> IpcServer
    IpcServiceModule --> DeviceManager
    DevicePoolModule --> DeviceManager
```
