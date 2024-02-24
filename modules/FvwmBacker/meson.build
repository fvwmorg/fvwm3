module_sources = [
    'FvwmBacker.c',
    'root_bits.c',
]

executable(
    'FvwmBacker',
    module_sources,
    dependencies: all_found_deps,
    include_directories: includedirs,
    install_dir: fvwm_moduledir,
    install: true,
    link_with: libfvwm3,
)

install_data('ConfigFvwmBacker', install_dir: fvwm_datadir)
