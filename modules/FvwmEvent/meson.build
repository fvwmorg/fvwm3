module_sources = [
  'FvwmEvent.c'
]

fvwmauto = executable(
  'FvwmEvent',
  module_sources,
  include_directories: [lib_includes, fvwm_includes],
  dependencies: all_found_deps,
  link_with: libraries,
)
