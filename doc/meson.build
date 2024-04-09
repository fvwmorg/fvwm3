adoc_sources = [
    'FvwmAnimate.adoc',
    'FvwmAuto.adoc',
    'FvwmBacker.adoc',
    'FvwmButtons.adoc',
    'FvwmEvent.adoc',
    'FvwmForm.adoc',
    'FvwmIconMan.adoc',
    'FvwmIdent.adoc',
    'FvwmMFL.adoc',
    'FvwmPerl.adoc',
    'FvwmRearrange.adoc',
    'FvwmScript.adoc',
    'fvwm-convert-2.6.adoc',
    'fvwm-menu-desktop.adoc',
    'fvwm-menu-directory.adoc',
    'fvwm-menu-xlock.adoc',
    'fvwm-perllib.adoc',
    'fvwm-root.adoc',
    'fvwm3.adoc',
    'fvwm3all.adoc',
    'fvwm3commands.adoc',
    'fvwm3menus.adoc',
    'fvwm3styles.adoc',
    'FvwmPager.adoc',
]


html_pages = []
if get_option('htmldoc')

    htmldir = join_paths(
        get_option('datadir'),
        'doc',
        meson.project_name() + '-' + meson.project_version(),
        'html',
    )

    # We need to configure index.html here to strip out whichever modules are unused.
    # This is done by using sed to remove the lines that contain the module names.
    index_src = 'index.adoc.in'
    index_dst = 'index.adoc'
    if golang.found()
        index = configure_file(
            input: index_src,
            output: index_dst,
            command: [
                sed,
                '-e', '/FvwmConsole.adoc/d;/FvwmCommand.adoc/d',
                '@INPUT@',
            ],
            capture: true,
        )
        adoc_sources += ['FvwmPrompt.adoc']
    else
        index = configure_file(
            input: index_src,
            output: index_dst,
            command: [sed, '-e', '/FvwmConsole.adoc/d', '@INPUT@'],
            capture: true,
        )
        adoc_sources += ['FvwmConsole.adoc', 'FvwmCommand.adoc']
    endif

    html_sources = adoc_sources

    foreach source : html_sources
        dest = source.replace('.adoc', '.html')
        html_pages += custom_target(
            dest,
            input: source,
            output: dest,
            command: [asciidoc, '-b', 'html5', '-o', '@OUTPUT@', '@INPUT@'],
            install: true,
            install_dir: htmldir,
        )
    endforeach

    custom_target(
        'index.html',
        input: index,
        output: 'index.html',
        command: [asciidoc, '-b', 'html5', '-o', '@OUTPUT@', '@INPUT@'],
        install: true,
        install_dir: htmldir,
    )
endif

man_pages = []
if get_option('mandoc')
    # Each of the mans in the current makefile gets installed to man1
    foreach source : adoc_sources
        dest = source.replace('.adoc', '.1')
        man_pages += custom_target(
            dest,
            input: source,
            output: dest,
            command: [asciidoc, '-b', 'manpage', '-o', '@OUTPUT@', '@INPUT@'],
            install: true,
            install_dir: join_paths(get_option('mandir'), 'man1'),
        )
    endforeach
endif
