# SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

gui_sources = files(
  'src/main.cpp'
)

executable(
  'wiregate-gui',
  gui_sources,
  dependencies : [ipc_dep, json_dep, crow_dep],
  install : false
)
