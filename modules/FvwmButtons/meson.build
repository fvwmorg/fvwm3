module_sources = [
  'FvwmButtons.c',
  'button.c',
  'draw.c',
  'dynamic.c',
  'icons.c',
  'misc.c',
  'output.c',
  'parse.c',
]

fvwmauto = executable(
  'FvwmButtons',
  module_sources,
  include_directories: [lib_includes, fvwm_includes],
  dependencies: all_found_deps,
  link_with: libraries,
)