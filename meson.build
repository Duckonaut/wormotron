# wormotron consists of 4 modules:
# - the fantasy console: wormotron
# - the assembler: weave
# - the virtual machine: squirm
# - the assembly spec: burrow
# - the shared util library: utils
#
# all 4 modules are written in C20.
#
# wormotron is a SDL2-based fantasy console that uses a custom
# bytecode format `burrow` for programs.
#
# - `squirm` is a virtual machine that runs `burrow` bytecode.
#   It's built as a library to be embedded in `wormotron` or
#   test programs, but also offers a command-line interface.
# - `weave` is an assembler that compiles `burrow` source code
#   into `burrow` bytecode. It can be built as either a library
#   or a command-line tool.
# - `wormotron` is the fantasy console itself. It depends directly
#   on `squirm` and `weave` to run programs. It also depends on
#   SDL2 for graphics, audio, and input.

project('wormotron', 'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'c_std=c11',
  ],
)

# setup C compiler

cc = meson.get_compiler('c')

static_link_libs = false
if host_machine.system() == 'windows'
  static_link_libs = true
endif

if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  add_project_arguments('-Wconversion', '-Werror', '-g', language : 'c')
endif

if get_option('buildtype') == 'release'
  add_project_arguments('-DRELEASE', language : 'c')
endif

# setup SDL2

sdl2 = dependency('sdl2', required : true, static : static_link_libs)

# setup `squirm` as subproject

squirm = subproject('squirm')

# setup `weave` as subproject

weave = subproject('weave')

# setup `burrow` as subproject

burrow = subproject('burrow')

# setup `utils` as subproject

utils = subproject('utils')

# setup `wormotron` as main target

wt_src = [
  'src/main.c',
  'src/rom.c',
  'src/graphics.c',
  'src/wormotron.c',
]

wt_inc = [
  'include',
]

wt_deps = [
  sdl2,
  squirm.get_variable('squirm_dep'),
  weave.get_variable('weave_dep'),
  utils.get_variable('utils_dep'),
]

wt = executable('wormotron', wt_src,
  include_directories : wt_inc,
  dependencies : wt_deps,
)
