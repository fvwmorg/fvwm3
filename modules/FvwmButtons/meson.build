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

executable(
    'FvwmButtons',
    module_sources,
    include_directories: includedirs,
    dependencies: all_found_deps,
    link_with: libfvwm3,
    install_dir: fvwm_moduledir,
)
