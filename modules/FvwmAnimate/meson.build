module_fvwmanimate = ['FvwmAnimate.c']

executable(
    'FvwmAnimate',
    module_fvwmanimate,
    include_directories: [lib_includes, fvwm_includes],
    dependencies: all_found_deps,
    link_with: libfvwm3,
    install_dir: fvwm_moduledir,
)
