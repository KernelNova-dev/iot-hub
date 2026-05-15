#include "logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <string>
#include <utility>

namespace core {

namespace {

spdlog::level::level_enum to_spd_level(LogLevel level) {
  switch (level) {
    case LogLevel::trace: return spdlog::level::trace;
    case LogLevel::debug: return spdlog::level::debug;
    case LogLevel::info: return spdlog::level::info;
    case LogLevel::warn: return spdlog::level::warn;
    case LogLevel::err: return spdlog::level::err;
    case LogLevel::critical: return spdlog::level::critical;
  }
  return spdlog::level::info;
}

std::shared_ptr<spdlog::logger> get_or_create_logger(std::string_view name) {
  const auto logger_name = std::string{name};

  if (auto logger = spdlog::get(logger_name)) {
    return logger;
  }

  return spdlog::stdout_color_mt(logger_name);
}

} // namespace

Logger Logger::getLogger(std::string_view module, LogLevel level) {
  auto logger = get_or_create_logger(module);
  logger->set_level(to_spd_level(level));
  logger->set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
  spdlog::flush_on(spdlog::level::info);

  return Logger{logger};
}

Logger::Logger(std::shared_ptr<spdlog::logger> logger)
    : logger_{std::move(logger)} {
}

void Logger::trace(std::string_view message) const {
  logger_->trace("{}", message);
}

void Logger::debug(std::string_view message) const {
  logger_->debug("{}", message);
}

void Logger::info(std::string_view message) const {
  logger_->info("{}", message);
}

void Logger::warn(std::string_view message) const {
  logger_->warn("{}", message);
}

void Logger::error(std::string_view message) const {
  logger_->error("{}", message);
}

void Logger::critical(std::string_view message) const {
  logger_->critical("{}", message);
}

} // namespace core
