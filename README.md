<!--
SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

## WireGate

Toolset to track and manage connected devices

## Project structure

``` 
project-root/
├── AUTHORS.md
├── config/
│   ├── wiregate-config.json
│   └── wiregate-web.json
├── docs/
├── src/
│   ├── cli/
│   ├── core/
│   ├── gui/
│   ├── ipc/
│   └── service/
├── tests/
│   └── meson.build
├── tools/
│   ├── build.sh
│   └── profiles/
│       └── profile.default
├── LICENSE
├── meson.build
├── conanfile.py
```

## Build and package

```bash
./tools/build.sh build
./tools/build.sh package
```

The package command creates a runnable application layout under `build/package/wiregate/`
and an archive named `build/package/wiregate-<os>-<arch>.tar.gz`.

## CLI usage

```bash
./cli health
./cli status
./cli list
./cli list --type thermostat
./cli get thermostat-001
./cli telemetry thermostat-001
./cli commands
./cli command relay-001 turn_on
./cli watch list --interval 2
```

Use `--socket <path>` when the service uses a non-default IPC socket.

## Web UI

```bash
./run-wiregate-service.sh
./gui
```

The web interface starts on `http://127.0.0.1:8080` by default.

## License and authorship

WireGate is free and open source software licensed under the
GNU General Public License v3.0 or later (`GPL-3.0-or-later`).
See [LICENSE](./LICENSE) for the full license text.

Copyright is retained by Daryna Vasylchenko (`KernelNova`).
See [AUTHORS.md](./AUTHORS.md) for author information.

Source files use SPDX headers to make ownership and license terms explicit.
JSON configuration files keep valid JSON syntax and use adjacent `.license`
sidecar files for the same SPDX metadata.
