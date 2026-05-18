<!--
SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# WireGate Abstraction Relations

This diagram intentionally hides most implementation detail and shows the main dependency-inversion boundaries.

![WireGate abstraction relations](./class-diagram-abstractions.svg)

```mermaid
classDiagram
    direction TB

    class Service {
        <<interface>>
        installModule(module)
        startMainLoop(settings)
        shutdown()
    }

    class WireGateService {
        <<implementation>>
    }

    class ServiceModule {
        <<extension point>>
        initialize(settings)
        start(context)
        execute(context)
        shutdown()
    }

    class IpcServiceModule {
        <<active module>>
    }

    class DevicePoolModule {
        <<tick module>>
    }

    class DeviceManager {
        <<interface>>
        list()
        find(id)
        sendCommand(id, command)
    }

    class DeviceRegistry {
        <<implementation>>
    }

    class IpcServer {
        <<infrastructure>>
    }

    Service <|-- WireGateService
    ServiceModule <|-- IpcServiceModule
    ServiceModule <|-- DevicePoolModule
    DeviceManager <|-- DeviceRegistry

    WireGateService --> ServiceModule : lifecycle calls
    WireGateService --> DeviceManager : owns abstraction
    IpcServiceModule --> DeviceManager : depends on abstraction
    DevicePoolModule --> DeviceManager : depends on abstraction
    IpcServiceModule --> IpcServer : adapts infrastructure
```
