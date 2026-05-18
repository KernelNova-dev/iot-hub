# SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

project(
  'wiregate',
  'cpp',
  version: '1.0',
  default_options: [
    'cpp_std=c++23',
    'warning_level=3'
  ]
)

asio_dep = dependency('asio')
json_dep = dependency('nlohmann_json')
spdlog_dep = dependency('spdlog')
cli11_dep = dependency('CLI11')
crow_dep = dependency('crowcpp-crow')

subdir('src')
#subdir('tests')
