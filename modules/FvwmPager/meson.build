install_data(
    'ConfigFvwmPager',
    install_dir: join_paths(datadir, meson.project_name()),
)

executable(
    'FvwmPager',
    sources: ['FvwmPager.c', 'x_pager.c'],
    dependencies: [all_found_deps],
    include_directories: [lib_includes, fvwm_includes],
    install: true,
    link_with: libraries,
    install_dir: fvwm_moduledir,
)
