project(
  'zsurface',
  'c', 'cpp',
  license : 'Apache-2.0',
  default_options : ['warning_level=3', 'werror=true', 'cpp_std=c++17', 'optimization=2'],
  meson_version : '>= 0.57.0',
  version : '0.0.1',
)

public_inc = include_directories('include')

wayland_client_dep = dependency('wayland-client')

subdir('protocol')
subdir('zsurface')
subdir('example')
