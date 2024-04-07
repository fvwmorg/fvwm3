module_sources = ['FvwmAuto.c']

executable(
    'FvwmAuto',
    module_sources,
    include_directories: includedirs,
    dependencies: all_found_deps,
    link_with: libfvwm3,
    install_dir: fvwm_moduledir,
)
