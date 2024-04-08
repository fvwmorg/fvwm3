# We only install FvwmCommand for non-golang builds.
# Since this isn't currently configurable, we'll just check for the presence of golang
# at build time. Not ideal, but we should probably drop FvwmCommand anyway.
if golang.found()
    subdir('FvwmPrompt')
else
    FvwmCommand = configure_file(
        input: 'FvwmCommand.in',
        output: 'FvwmCommand',
        configuration: python_config,
        install: true,
        install_dir: bindir,
        install_mode: install_mask_755,
    )
endif

# We should tighten up the dependencies here, but for now we'll just use all_found_deps.
fvwm_root = executable(
    'fvwm-root',
    'fvwm-root.c',
    include_directories: includedirs,
    link_with: libfvwm3,
    dependencies: all_found_deps,
    install: true,
)

fvwm_perllib = configure_file(
    input: 'fvwm-perllib.in',
    output: 'fvwm-perllib',
    configuration: perl_config,
    install: true,
    install_dir: bindir,
    install_mode: install_mask_755,
)

fvwm_convert = configure_file(
    input: 'fvwm-convert-2.6.in',
    output: 'fvwm-convert-2.6',
    configuration: perl_config,
    install: true,
    install_dir: bindir,
    install_mode: install_mask_755,
)

fvwm_menu_xlock = configure_file(
    input: 'fvwm-menu-xlock.in',
    output: 'fvwm-menu-xlock',
    configuration: perl_config,
    install: true,
    install_dir: bindir,
    install_mode: install_mask_755,
)

fvwm_menu_directory = configure_file(
    input: 'fvwm-menu-directory.in',
    output: 'fvwm-menu-directory',
    configuration: perl_config,
    install: true,
    install_dir: bindir,
    install_mode: install_mask_755,
)

fvwm_menu_desktop = configure_file(
    input: 'fvwm-menu-desktop.in',
    output: 'fvwm-menu-desktop',
    configuration: python_config,
    install: true,
    install_dir: bindir,
    install_mode: install_mask_755,
)

config_data = install_data(
    'fvwm-menu-desktop-config.fpl',
    install_dir: fvwm_datadir,
)
