configure_file(
    input: 'FvwmCommandS.in',
    output: 'FvwmCommandS',
    configuration: conf,
    install_dir: fvwm_moduledir,
    install_mode: install_mask_755,
)

executable(
    'FvwmMFL',
    sources: 'FvwmMFL.c',
    dependencies: [all_found_deps],
    include_directories: includedirs,
    install_dir: fvwm_moduledir,
    install: true,
    link_with: libfvwm3,
)
