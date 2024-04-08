module_fvwmanimate = ['FvwmAnimate.c']

executable(
    'FvwmAnimate',
    module_fvwmanimate,
    dependencies: all_found_deps,
    include_directories: includedirs,
    install_dir: fvwm_moduledir,
    install: true,
    link_with: libfvwm3,
)
