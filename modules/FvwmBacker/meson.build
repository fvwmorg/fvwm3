module_sources = [
  'FvwmBacker.c',
  'root_bits.c',
]

fvwmauto = executable(
  'FvwmBacker',
  module_sources,
  include_directories: [lib_includes, fvwm_includes],
  dependencies: all_found_deps,
  link_with: libraries,
)