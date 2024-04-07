# TODO: Tidy up the dependencies
if not golang.found()
    executable(
        'FvwmConsole',
        sources: ['FvwmConsole.c', 'FvwmConsole.h'],
        dependencies: [all_found_deps],
        include_directories: [lib_includes, fvwm_includes],
        install: true,
        link_with: libraries,
        install_dir: fvwm_moduledir,
    )
    executable(
        'FvwmConsoleC',
        sources: ['FvwmConsoleC.c', 'FvwmConsole.h', 'getline.c'],
        dependencies: [all_found_deps],
        include_directories: [lib_includes, fvwm_includes],
        install: true,
        link_with: libraries,
        install_dir: fvwm_moduledir,
    )
endif
