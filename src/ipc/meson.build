ipc_inc = include_directories('include')
ipc_sources = files(
  'src/IpcClient.cpp',
  'src/IpcMessage.cpp',
  'src/IpcProtocol.cpp',
  'src/IpcServer.cpp',
  'src/IpcSession.cpp'
)

ipc_lib = shared_library(
  'ipc',
  ipc_sources,
  include_directories : ipc_inc,
  dependencies : [asio_dep, json_dep],
  install : false,
)

ipc_dep = declare_dependency(
  link_with : ipc_lib,
  include_directories : ipc_inc,
  dependencies : [asio_dep, json_dep]
)
