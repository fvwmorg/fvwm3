iconman_sources = [
    'FvwmIconMan.c',
    'debug.c',
    'debug.h',
    'functions.c',
    'fvwm.c',
    'globals.c',
    'readconfig.c',
    'winlist.c',
    'x.c',
    'xmanager.c',
]

executable(
    'FvwmIconMan',
    sources: iconman_sources,
    dependencies: [all_found_deps],
    include_directories: includedirs,
    install_dir: fvwm_moduledir,
    install: true,
    link_with: libfvwm3,
)

install_data('ConfigFvwmIconMan', install_dir: fvwm_datadir)
