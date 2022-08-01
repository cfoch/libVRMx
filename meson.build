project('libVRMx', 'cpp', version: '0.0.0', meson_version: '>= 0.59')

tinygltf_subproject = subproject('tinygltf')
tinygltf_inc = tinygltf_subproject.get_variable('tinygltf_inc')

gl_dep = dependency('gl', required: true)
glfw_dep = dependency('glfw3', required: true)

subdir('src')
subdir('examples')