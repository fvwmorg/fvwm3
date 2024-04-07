

install_data(
    'ConfigFvwmIdent',
    install_dir: join_paths(datadir, meson.project_name()),
)

executable(
    'FvwmIdent',
    sources: 'FvwmIdent.c',
    dependencies: [all_found_deps],
    include_directories: includedirs,
    install: true,
    link_with: libfvwm3,
    install_dir: fvwm_moduledir,
)
