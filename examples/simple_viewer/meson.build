gtk_dep = dependency('gtk4')

if gtk_dep.found()
  executable('simple-viewer',
    sources: 'simple-viewer.cpp',
    cpp_args: ['-DPROG_DIR="@0@"'.format(meson.current_source_dir())],
    include_directories: [tinygltf_inc, magic_enum_inc],
    dependencies: [libvrmx_dep, gtk_dep],
  )
endif