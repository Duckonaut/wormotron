# small header-only util library for use in the other C projects in this repo

project('utils', 'c',
  version : '0.1.0',
  default_options : ['warning_level=3', 'c_std=c11'],
)

util_inc_dirs = [
  'include',
]

util_inc = include_directories(util_inc_dirs)

util_src = [
  'src/log.c',
  'src/utils.c',
]

utils_lib = static_library('utils',
  util_src,
  include_directories : util_inc,
)

utils_dep = declare_dependency(
  include_directories : util_inc,
  link_with : utils_lib,
)
