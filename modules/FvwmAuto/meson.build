module_sources = ['FvwmAuto.c']

executable(
    'FvwmAuto',
    module_sources,
    dependencies: all_found_deps,
    include_directories: includedirs,
    install_dir: fvwm_moduledir,
    install: true,
    link_with: libfvwm3,
)
