perl_FVWMmod = [
    'FVWM/Constants.pm',
    'FVWM/Event.pm',
    'FVWM/EventNames.pm',
    'FVWM/Tracker.pm',
    'FVWM/Commands.pm',
    'FVWM/Module/Terminal.pm',
    'FVWM/Module/Toolkit.pm',
    'FVWM/Tracker/Colorsets.pm',
    'FVWM/Tracker/GlobalConfig.pm',
    'FVWM/Tracker/ModuleConfig.pm',
    'FVWM/Tracker/PageInfo.pm',
    'FVWM/Tracker/Scheduler.pm',
    'FVWM/Tracker/WindowList.pm',
]

perl_FVWMmodfile = [
    'FVWM/Module.pm.in',
]

configure_file(
    input: perl_FVWMmodfile,
    output: 'Module.pm',
    configuration: file_config,
    install_dir: join_paths(file_config.get_unquoted('FVWM_PERLLIBDIR'), 'FVWM'),
    install: true,
)

install_data(
    perl_FVWMmod,
    install_dir: file_config.get_unquoted('FVWM_PERLLIBDIR'),
    preserve_path: true,
)

perl_general_module = [
    'General/FileSystem.pm',
    'General/Parse.pm',
]

install_data(
    perl_general_module,
    install_dir: file_config.get_unquoted('FVWM_PERLLIBDIR') + '/General',
)
