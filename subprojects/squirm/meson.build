# `squirm` is a virtual machine for the `tangle` language.
# It is built as a library.

project('squirm', 'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'c_std=c11'
  ],
)

squirm_inc_dirs = [
  'include',
]

squirm_inc = include_directories(squirm_inc_dirs)

squirm_src = [
  'src/squirm.c',
]

squirm_bin_src = [
  squirm_src,
  'src/main.c',
  'src/squirm_dbg.c',
]

utils = subproject('utils')

utils_dep = utils.get_variable('utils_dep')

burrow = subproject('burrow')

burrow_dep = burrow.get_variable('burrow_dep')

squirm_deps = [
  utils_dep,
  burrow_dep,
]

squirm_lib = static_library('squirm',
  squirm_src,
  include_directories: squirm_inc,
  dependencies: squirm_deps,
)

squirm_dep = declare_dependency(
  include_directories: squirm_inc,
  dependencies: squirm_deps,
  link_with: squirm_lib,
)

executable('squirm',
  squirm_bin_src,
  include_directories: squirm_inc,
  dependencies: squirm_deps,
)
