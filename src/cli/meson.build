# SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cli_sources = files(
  'src/main.cpp'
)

executable(
  'wiregate-cli',
  cli_sources,
  dependencies : [ipc_dep, cli11_dep],
  install : false
)
