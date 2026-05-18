# SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

core_inc = include_directories('include')


core_sources = files(
  'src/logger.cpp'
)

core_lib = shared_library(
  'wiregate-core',
  core_sources,
  include_directories : core_inc,
  dependencies : [spdlog_dep],
  install : false
)

core_dep = declare_dependency(
  link_with : core_lib,
  include_directories : core_inc,
  dependencies : [spdlog_dep]
)
