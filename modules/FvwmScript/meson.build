libwidget_sources = [
    'Widgets/CheckBox.c',
    'Widgets/HDipstick.c',
    'Widgets/HScrollBar.c',
    'Widgets/ItemDraw.c',
    'Widgets/List.c',
    'Widgets/Menu.c',
    'Widgets/MiniScroll.c',
    'Widgets/PopupMenu.c',
    'Widgets/PushButton.c',
    'Widgets/RadioButton.c',
    'Widgets/Rectangle.c',
    'Widgets/Swallow.c',
    'Widgets/TextField.c',
    'Widgets/Tools.c',
    'Widgets/Tools.h',
    'Widgets/VDipstick.c',
    'Widgets/VScrollBar.c',
    'Widgets/Widget.c',
]

# Todo: could/should this be a shared library?
libWidgets = static_library(
    'Widgets',
    sources: libwidget_sources,
    dependencies: all_found_deps,
    include_directories: includedirs,
    install_dir: get_option('libdir'),
    link_with: libfvwm3,
)

fvwmscript_sources = [
    'FvwmScript.c',
    'Instructions.c',
    'libyywrap.c',
    'scanner.c',
    'script.c',
]

executable(
    'FvwmScript',
    sources: fvwmscript_sources,
    dependencies: all_found_deps,
    include_directories: includedirs,
    install_dir: fvwm_moduledir,
    install: true,
    link_with: [libfvwm3, libWidgets],
)

install_scripts = [
    'Scripts/FvwmScript-BaseConfig',
    'Scripts/FvwmScript-BellSetup',
    'Scripts/FvwmScript-Buttons',
    'Scripts/FvwmScript-Colorset',
    'Scripts/FvwmScript-Date',
    'Scripts/FvwmScript-FileBrowser',
    'Scripts/FvwmScript-Find',
    'Scripts/FvwmScript-KeyboardSetup',
    'Scripts/FvwmScript-PointerSetup',
    'Scripts/FvwmScript-Quit',
    'Scripts/FvwmScript-ScreenDump',
    'Scripts/FvwmScript-ScreenSetup',
    'Scripts/FvwmScript-WidgetDemo',
]

install_data(
    install_scripts,
    install_dir: fvwm_datadir,
)

install_data(
    'Scripts/fvwm-script-ComExample.pl',
    install_dir: fvwm_datadir,
)

# This sed is pulled straight from the makefile; we can do better
# by directly using configure_file() with a proper template rather
# than this silly sed business.
perl_path = perl.full_path()
configure_file(
    input: 'Scripts/FvwmScript-ComExample.in',
    output: 'FvwmScript-ComExample',
    command: [
        sed,
        '-e', f's,xFVWM_DATADIRx,@fvwm_datadir@,',
        '-e', f's,xPERLx,@perl_path@,',
        '@INPUT@',
    ],
    capture: true,
    install_dir: join_paths(fvwm_datadir),
)

install_symlink(
    'FvwmScript-ConfirmCopyConfig',
    pointing_to: 'default-config/FvwmScript-ConfirmCopyConfig',
    install_dir: fvwm_datadir,
)

install_symlink(
    'FvwmScript-ConfirmQuit',
    pointing_to: 'default-config/FvwmScript-ConfirmQuit',
    install_dir: fvwm_datadir,
)

install_symlink(
    'FvwmScript-DateTime',
    pointing_to: 'default-config/FvwmScript-DateTime',
    install_dir: fvwm_datadir,
)
