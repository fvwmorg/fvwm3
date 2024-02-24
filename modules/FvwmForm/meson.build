install_data = [
    'FvwmForm-Form',
    'FvwmForm-Capture',
    'FvwmForm-Desktop',
    'FvwmForm-QuitVerify',
    'FvwmForm-Rlogin',
    'FvwmForm-RootCursor',
    'FvwmForm-Talk',
    'FvwmForm-TalkHelp',
    'FvwmForm-XDGOptionsHelp',
    'FvwmForm-XDGMenuHelp',
]

install_data(
    install_data,
    install_dir: join_paths(get_option('datadir'), meson.project_name()),
)

fvwmform_sources = [
    'FvwmForm.c',
    'ReadXServer.c',
    'ParseCommand.c',
]

# TODO: Tidy up the dependencies
executable(
    'FvwmForm',
    fvwmform_sources,
    dependencies: [all_found_deps],
    include_directories: includedirs,
    install_dir: fvwm_moduledir,
    install: true,
    link_with: libfvwm3,
)

configure_file(
    input: 'FvwmTalk-wrapper.in',
    output: 'FvwmTalk',
    configuration: conf,
    install_dir: fvwm_moduledir,
    install_mode: install_mask_755,
)
