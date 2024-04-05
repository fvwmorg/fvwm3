install_data(
    'ConfigFvwmPager',
    install_dir: join_paths(datadir, meson.project_name()),
)

fvwmpager_sources = [
    'fpmonitor.c',
    'FvwmPager.c',
    'FvwmPager.h',
    'init_pager.c',
    'messages.c',
    'x_pager.c',
    'x_update.c',
]

executable(
    'FvwmPager',
    sources: fvwmpager_sources,
    dependencies: [all_found_deps],
    include_directories: includedirs,
    install_dir: fvwm_moduledir,
    install: true,
    link_with: libfvwm3,
)
