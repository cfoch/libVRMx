sources = [
  'vrmx.cpp',
  'vrmx-io.cpp'
]

config_data = configuration_data()
config_data.set('VERTEX_SHADER_SRC', vert_shader_data)
config_data.set('FRAGMENT_SHADER_SRC', frag_shader_data)

configure_file(output: 'config.h', configuration: config_data)

libvrmx_deps = [spdlog_dep, gl_dep, glu_dep, epoxy_dep]

libvrmx = library('libvrmx',
  sources: sources,
  include_directories: tinygltf_inc,
  include_directories: [tinygltf_inc, magic_enum_inc],
  dependencies: libvrmx_deps
)

libvrmx_dep = declare_dependency(
  link_with: libvrmx,
  include_directories: include_directories('.'),
  dependencies: libvrmx_deps,
)