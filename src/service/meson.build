service_inc = include_directories('include')
service_sources = files(
  'src/Service.cpp'
)

service_lib = shared_library(
  'iot-hub-service-lib',
  service_sources,
  include_directories : service_inc,
  dependencies : [core_dep, ipc_dep, json_dep],
  install : false
)

service_dep = declare_dependency(
  link_with : service_lib,
  include_directories : service_inc,
  dependencies : [core_dep, ipc_dep, json_dep]
)

executable(
  'iot-hub-service',
  'src/main.cpp',
  include_directories : service_inc,
  dependencies : [service_dep],
  install : false
)
