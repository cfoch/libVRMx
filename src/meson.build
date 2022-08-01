sources = [
  'vrmx.cpp',
]

libvrmx = library('libvrmx',
  sources: sources,
  include_directories: tinygltf_inc,
)

libvrmx_dep = declare_dependency(
  link_with: libvrmx,
  include_directories: include_directories('.'))