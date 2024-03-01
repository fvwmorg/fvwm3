module_sources = [
  'FvwmAuto.c'
]

fvwmauto = executable(
  'FvwmAuto',
  module_sources,
  include_directories: [lib_includes, fvwm_includes],
  dependencies: all_found_deps,
  link_with: libraries,
)