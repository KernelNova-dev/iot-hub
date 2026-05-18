# SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

service_inc = include_directories('include')

service_sources = files(
  'src/Device.cpp',
  'src/DeviceRegistry.cpp',
  'src/DevicePoolModule.cpp',
  'src/IpcServiceModule.cpp',
  'src/Service.cpp',
  'src/WireGateService.cpp',
  'src/main.cpp'
)

service_dep = [core_dep, ipc_dep, json_dep]

executable(
  'wiregate-service',
  service_sources,
  include_directories : service_inc,
  dependencies : [service_dep],
  install : false
)
