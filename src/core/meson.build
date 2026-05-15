core_inc = include_directories('include')


core_sources = files(
  'src/logger.cpp'
)

core_lib = shared_library(
  'iot-hub-core',
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
