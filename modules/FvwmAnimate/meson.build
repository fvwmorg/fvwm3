module_fvwmanimate = ['FvwmAnimate.c']

executable(
    'FvwmAnimate',
    module_fvwmanimate,
    include_directories: [lib_includes, fvwm_includes],
    dependencies: all_found_deps,
    link_with: libraries,
    install_dir: fvwm_moduledir,
)
