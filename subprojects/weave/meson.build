# `weave` is an assembler for the `burrow` language.
# It's a subproject of `wormotron` and exposes both
# a library and a command line interface.

project('weave', 'c',
  version : '0.1.0',
  default_options : ['warning_level=3', 'c_std=c2x'],
)

weave_src = [
  'src/weave.c',
  'src/lexer.c',
  'src/preprocessor.c',
]

weave_bin_src = [
  weave_src,
  'src/main.c',
]

weave_inc = include_directories('include')

utils = subproject('utils')

utils_dep = utils.get_variable('utils_dep')

burrow = subproject('burrow')

burrow_dep = burrow.get_variable('burrow_dep')

weave_deps = [
  utils_dep,
  burrow_dep,
]

weave_lib = static_library('weave',
  weave_src,
  include_directories : weave_inc,
  dependencies : weave_deps
)

executable('weave', weave_bin_src,
  include_directories : weave_inc,
  dependencies : weave_deps
)

weave_dep = declare_dependency(
  include_directories : weave_inc,
  dependencies : weave_deps,
  link_with : weave_lib,
)
