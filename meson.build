project('libVRMx', 'cpp',
        version: '0.0.0',
        default_options: ['cpp_std=c++17'],
        meson_version: '>= 0.59')

tinygltf_subproject = subproject('tinygltf')
tinygltf_inc = tinygltf_subproject.get_variable('tinygltf_inc')
magic_enum_subproject = subproject('magic_enum')
magic_enum_inc = magic_enum_subproject.get_variable('magic_enum_include')
spdlog = subproject('spdlog')
spdlog_dep = spdlog.get_variable('spdlog_dep')

gl_dep = dependency('gl', required: true)
glfw_dep = dependency('glfw3', required: true)

subdir('src')
subdir('bin')
subdir('examples')