// SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string_view>

namespace spdlog {
class logger;
} // namespace spdlog

namespace wiregate::core {

enum class LogLevel {
  trace,
  debug,
  info,
  warn,
  err,
  critical
};

class Logger {
public:
  static Logger getLogger(std::string_view module, LogLevel level = LogLevel::info);

  void trace(std::string_view message) const;
  void debug(std::string_view message) const;
  void info(std::string_view message) const;
  void warn(std::string_view message) const;
  void error(std::string_view message) const;
  void critical(std::string_view message) const;

private:
  explicit Logger(std::shared_ptr<spdlog::logger> logger);

  std::shared_ptr<spdlog::logger> logger_;
};

} // namespace wiregate::core
