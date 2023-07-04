# small header spec of the `burrow` assembly code

project('burrow', 'c',
  version : '0.1.0',
  default_options : ['warning_level=3', 'c_std=c11'],
)

burrow_inc_dirs = [
  'include',
]

utils = subproject('utils')

utils_dep = utils.get_variable('utils_dep')

burrow_inc = include_directories(burrow_inc_dirs)

burrow_deps = [
  utils_dep,
]

burrow_dep = declare_dependency(
  include_directories : burrow_inc,
  dependencies : burrow_deps,
)
