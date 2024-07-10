adoc_sources = [
    'fvwm-convert-2.6.adoc',
    'fvwm-menu-desktop.adoc',
    'fvwm-menu-directory.adoc',
    'fvwm-menu-xlock.adoc',
    'fvwm-perllib.adoc',
    'fvwm-root.adoc',
    'fvwm3.adoc',
    'fvwm3all.adoc',
    'FvwmAnimate.adoc',
    'FvwmAuto.adoc',
    'FvwmBacker.adoc',
    'FvwmButtons.adoc',
    'FvwmCommand.adoc',
    'FvwmEvent.adoc',
    'FvwmForm.adoc',
    'FvwmIconMan.adoc',
    'FvwmIdent.adoc',
    'FvwmMFL.adoc',
    'FvwmPager.adoc',
    'FvwmPerl.adoc',
    'FvwmRearrange.adoc',
    'FvwmScript.adoc',
]

# The adoc files in the repo for fvwm3-commands, fvwm3-menus, and fvwm3-styles
# are generated from the main manpage source file. We need to extract the
# relevant sections from the main file to generate the source file includes.
fs = import('fs')
main_file = 'fvwm3_manpage_source.adoc'

# Custom build command for each source file
ad_build_targets = []
sections = ['commands', 'menus', 'styles']

foreach sec : sections
    outfile = f'fvwm3_@sec@.ad'
    # We could use a generator here, but since multiple targets include
    # the same file we would end up generating multiple times. Instead
    # Just build each section once and include it in the targets that need it.
    ad_build_targets += custom_target(
        output: outfile,
        input: main_file,
        command: ['generate_ad_file.sh', sec, '@INPUT@'],
        capture: true,
        install: false,
    )
endforeach

# We need to copy the stub adoc files to the build directory
# so that we can generate docs from them; asciidoc includes
# the files by path, so we need to make sure they are in the
# same directory as the generated includes.
needs_generated_source = [
    'fvwm3commands.adoc',
    'fvwm3menus.adoc',
    'fvwm3styles.adoc',
]

late_docs_targets = []
foreach src : needs_generated_source
    late_docs_targets += fs.copyfile(src)
endforeach

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
                '-e', '/FvwmConsole.adoc/d',
                '@INPUT@',
            ],
            capture: true,
        )
        adoc_sources += ['FvwmPrompt.adoc']
    else
        index = configure_file(
            input: index_src,
            output: index_dst,
            command: [sed, '-e', '/FvwmPrompt.adoc/d', '@INPUT@'],
            capture: true,
        )
        adoc_sources += ['FvwmConsole.adoc']
    endif

    html_sources = adoc_sources

    foreach source : html_sources
        dest = source.replace('.adoc', '.html')
        doc_name_attribute = source.replace(
            '.adoc',
            '',
        )
        html_pages += custom_target(
            dest,
            input: source,
            output: dest,
            command: [
                asciidoc,
                '-b', 'html5',
                '-a', 'toc',
                '-a', 'webfonts!',
                '-a', doc_name_attribute,
                '-o', '@OUTPUT@',
                '@INPUT@',
            ],
            install: true,
            install_dir: htmldir,
        )
    endforeach

    foreach source : late_docs_targets
        # We can use the full_path method on a given custom_target to derive
        # the filename, then replace the extension to get our output.
        dest = source.full_path().split('/')[-1].replace('.adoc', '.html')
        doc_name_attribute = source.full_path().split('/')[-1].replace(
            '.adoc',
            '',
        )
        html_pages += custom_target(
            dest,
            input: source,
            output: dest,
            command: [
                asciidoc,
                '-b', 'html5',
                '-a', 'toc',
                '-a', 'webfonts!',
                '-a', doc_name_attribute,
                '-o', '@OUTPUT@',
                '@INPUT@',
            ],
            install: true,
            install_dir: htmldir,
            depends: ad_build_targets,
        )
    endforeach

    custom_target(
        'index.html',
        input: index,
        output: 'index.html',
        command: [
            asciidoc,
            '-b', 'html5',
            '-a', 'toc',
            '-a', 'webfonts!',
            '-a', 'index',
            '-o', '@OUTPUT@',
            '@INPUT@',
        ],
        install: true,
        install_dir: htmldir,
    )
endif

man_pages = []
if get_option('mandoc')
    # Each of the mans in the current makefile gets installed to man1
    foreach source : adoc_sources
        dest = source.replace('.adoc', '.1')
        doc_name_attribute = source.replace(
            '.adoc',
            '',
        )
        man_pages += custom_target(
            dest,
            input: source,
            output: dest,
            command: [
                asciidoc,
                '-b', 'manpage',
                '-a', doc_name_attribute,
                '-o', '@OUTPUT@',
                '@INPUT@',
            ],
            install: true,
            install_dir: join_paths(get_option('mandir'), 'man1'),
        )
    endforeach

    foreach source : late_docs_targets
        dest = source.full_path().split('/')[-1].replace('.adoc', '.1')
        doc_name_attribute = source.full_path().split('/')[-1].replace(
            '.adoc',
            '',
        )
        man_pages += custom_target(
            dest,
            input: source,
            output: dest,
            command: [
                asciidoc,
                '-b', 'manpage',
                '-a', doc_name_attribute,
                '-o', '@OUTPUT@',
                '@INPUT@',
            ],
            install: true,
            install_dir: join_paths(get_option('mandir'), 'man1'),
            depends: ad_build_targets,
        )
    endforeach
endif
