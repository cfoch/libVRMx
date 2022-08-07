sources = [
  'vrmx.cpp',
  'vrmx-io.cpp'
]

libvrmx = library('libvrmx',
  sources: sources,
  include_directories: tinygltf_inc,
  include_directories: [tinygltf_inc, magic_enum_inc],
  dependencies: spdlog_dep
)

libvrmx_dep = declare_dependency(
  link_with: libvrmx,
  include_directories: include_directories('.'),
  dependencies: spdlog_dep
)