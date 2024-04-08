# TODO: Tidy up the dependencies
if not golang.found()
    executable(
        'FvwmConsole',
        sources: ['FvwmConsole.c', 'FvwmConsole.h'],
        dependencies: [all_found_deps],
        include_directories: includedirs,
        install_dir: fvwm_moduledir,
        install: true,
        link_with: libfvwm3,
    )
    executable(
        'FvwmConsoleC',
        sources: ['FvwmConsoleC.c', 'FvwmConsole.h', 'getline.c'],
        dependencies: [all_found_deps],
        include_directories: includedirs,
        install_dir: fvwm_moduledir,
        install: true,
        link_with: libfvwm3,
    )
endif
