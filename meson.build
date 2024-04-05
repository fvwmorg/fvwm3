project(
    'fvwm3',
    'c',
    version: run_command(
        './utils/fvwm-version-str.sh',
        check: false,
    ).stdout().strip(),
    license: 'GPL',
    meson_version: '>=0.60.0',
    default_options: [
        'c_std=c11',
        'warning_level=2',
        'werror=false',
    ],
)

add_project_arguments(
    [
        '-Wno-implicit-int',
        '-fno-common',
        '-Wno-error=format-security',
        '-Wno-sign-compare',
        '-Wno-unused-parameter',
        '-Wno-extra',
    ],
    language: 'c',
)

cc = meson.get_compiler('c')
host = host_machine.system()

m_dep = cc.find_library('m', required: false)
if m_dep.found()
    add_project_link_arguments('-lm', language: 'c')
endif

datadir = get_option('datadir')
sysconfdir = get_option('sysconfdir')
prefix = get_option('prefix')

syslibs = []

conf = configuration_data()

if host_machine.endian() == 'big'
    conf.set('WORDS_BIGENDIAN', true)
endif

# Set static configuration.
conf.set_quoted('VERSION', '1.1.0')
conf.set_quoted('VERSIONINFO', meson.project_version())
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('FVWM2RC', '.fvwm2rc')
conf.set_quoted('FVWM_CONFIG', 'config')
conf.set_quoted(
    'FVWM_IMAGEPATH',
    '/usr/include/X11/bitmaps:/usr/include/X11/pixmaps',
)
conf.set_quoted(
    'FVWM_MODULEDIR',
    get_option('libexecdir') + '/' + meson.project_name() + '/' + meson.project_version(),
)
conf.set_quoted(
    'FVWM_DATADIR',
    get_option('datadir') + '/' + meson.project_name(),
)
conf.set_quoted('LOCALEDIR', conf.get('FVWM_DATADIR') + '/' + 'locale')
conf.set_quoted('FVWM_CONFDIR', get_option('sysconfdir'))
conf.set('RETSIGTYPE', 'void')
conf.set('SIGNAL_RETURN', 'return')
conf.set('ICONV_ARG_CONST', '')
conf.set('fd_set_size_t', 'int')
conf.set('EXECUTABLE_EXTENSION', 'NULL')

# Header checks.
headers = [
    'fcntl.h',
    'float.h',
    'getopt.h',
    'inttypes.h',
    'limits.h',
    'malloc.h',
    'memory.h',
    'stdarg.h',
    'stdint.h',
    'stdlib.h',
    'string.h',
    'sys/select.h',
    'sys/systeminfo.h',
    'sys/time.h',
    'sys/wait.h',
    'unistd.h',
]

foreach h : headers
    if cc.has_header(h)
        conf.set10('HAVE_' + h.to_upper().underscorify(), true)
    endif
endforeach

if conf.has('HAVE_SYS_SELECT_H')
    conf.set10('HAVE_SELECT', true)
    conf.set('SELECT_FD_SET_CAST', '')
endif

# Function checks.
functions = {
    'div': {},
    'getpwuid': {},
    'gettimeofday': {},
    'lstat': {},
    'memcpy': {},
    'memmove': {},
    'mkfifo': {},
    'nl_langinfo': {'have': 'HAVE_CODESET'},
    'putenv': {},
    'setpgid': {},
    'setpgrp': {},
    'setvbuf': {},
    'sigaction': {},
    'siginterrupt': {},
    'socket': {},
    'strchr': {},
    'strdup': {},
    'strstr': {},
    'strtol': {},
    'sysconf': {},
    'intl': {},
    'uname': {},
    'waitpid': {},
}

foreach f, v : functions
    if cc.has_function(f, dependencies: syslibs)
        val = ''
        if 'have' in v
            val = v['have']
        else
            val = 'HAVE_' + f.to_upper().underscorify()
        endif
        conf.set10(val, true)
    endif
endforeach

# Dependencies.
all_found_deps = []
all_req_deps = ['x11', 'xt', 'xrandr', 'libevent', 'ice']
all_optional_deps = {
    'xext': {'have': 'SHAPE', 'feature': true, 'opt': 'xent'},
    'libpng': {
        'version': '>=1.2',
        'opt': 'png',
        'disabled_msg': ''''***
      disabling PNG support will mean the default
      configuration shipped with FVWM will be without icon
      support.  Be sure this is what you mean to do.
      ***''',
        'not_found_msg': '''***
      libpng found, and -Dpng=false was given.
      It is recommended to use libpng so that PNG icons can
      be rendered by FVWM, which the default configuration
      file needs, as well as other third-party programs
      which generate menus, for example.

      If it is critical that FVWM does not link against
      libpng (perhaps due to size limitations of the
      binary), then use -Dpng=false
      ***''',
        'have': 'HAVE_PNG',
    },
    'readline': {'have': 'HAVE_READLINE'},
    'sm': {'have': 'SESSION'},
    'xcursor': {'have': 'HAVE_XCURSOR'},
    'xkbcommon-x11': {'have': 'HAVE_X11_XKBLIB_H'},
    'xpm': {'version': '>=2.0.0', 'have': 'HAVE_XPM'},
    'xrender': {'have': 'HAVE_XRENDER'},
    'freetype2': {
        'optional': {
            'xft': {'version': '>=2.3.0', 'required': true},
            'fontconfig': {'version': '>=2.0.0', 'required': true},
        },
        'have': 'HAVE_XFT',
    },
    'librsvg-2.0': {
        'have': 'HAVE_RSVG',
        'version': '>=2.13.92',
        'optional': {
            'x-one-of': {
                'cairo': {},
                'cairo-svg': {},
                'libsvg-cairo': {},
            },
        },
    },
    'fribidi': {
        'have': 'HAVE_BIDI',
        'version': '>=0.19.2',
    },
}

foreach rd : all_req_deps
    this_dep = dependency(rd, required: true)
    if not this_dep.found()
        msg = '@0@ libraries or header files could not be found.'.format(rd)
        error(msg)
    endif

    # If found, declare HAVE_* defines.
    conf.set10('HAVE_' + rd.to_upper().underscorify(), true)
    all_found_deps += this_dep
endforeach

foreach k, _ : all_optional_deps
    version = '>=0.0'
    is_feature = false
    required = false

    if 'version' in all_optional_deps[k]
        version = all_optional_deps[k]['version']
    endif

    if 'feature' in all_optional_deps[k]
        is_feature = true
        required = get_option(all_optional_deps[k]['opt'])
        this_dep = dependency(k, required: required)
    else
        if 'required' in all_optional_deps[k]
            required = all_optional_deps[k]['required']
        endif

        this_dep = dependency(k, required: required, version: version)
    endif

    # If the dependency is not found and we have custom messages, display them
    # and move on.
    if not this_dep.found()
        if 'disabled_msg' in all_optional_deps[k]
            message(all_optional_deps[k]['disabled_msg'])
            continue
        endif
    endif

    if this_dep.found()
        if (
            not is_feature
            and 'opt' in all_optional_deps[k]
            and not get_option(all_optional_deps[k]['opt'])
        )
            if 'not_found_msg' in all_optional_deps[k]
                message(all_optional_deps[k]['not_found_msg'])
            endif
            all_found_deps += this_dep
            continue
        endif
    endif

    if this_dep.found() and 'optional' not in all_optional_deps[k]
        all_found_deps += this_dep
        conf.set10(all_optional_deps[k]['have'], true)
        continue
    endif

    if this_dep.found() and 'optional' in all_optional_deps[k]
        all_found_deps += this_dep
        opt_found = false
        opt_found_names = []
        foreach ok, _ : all_optional_deps[k]['optional']
            op_ver = '>=0.0'
            if 'version' in all_optional_deps[k]['optional'][ok]
                op_ver = all_optional_deps[k]['optional'][ok]['version']
            endif

            required = false
            if 'required' in all_optional_deps[k]['optional'][ok]
                required = all_optional_deps[k]['optional'][ok]['required']
            endif

            if 'x-one-of' in all_optional_deps[k]['optional']
                # Special case (because there's not enough of these already!):
                #
                # In this instance, treat this as a set -- we need at least one in
                # order to succeed.
                xof_found = false
                xof_names = []
                foreach xof, _ : all_optional_deps[k]['optional']['x-one-of']
                    xof_names += xof
                    xof_ver = '>=0.0'
                    if (
                        'version' in all_optional_deps[k]['optional']['x-one-of'][xof]
                    )
                        xof_ver = all_optional_deps[k]['optional']['x-one-of'][xof]['version']
                    endif

                    xof_dep = dependency(
                        xof,
                        version: xof_ver,
                        required: false,
                    )

                    if xof_dep.found()
                        xof_found = true
                        all_found_deps += xof_dep
                        break
                    endif
                endforeach

                if not xof_found
                    p = ''
                    foreach n : xof_names
                        p += n + ' '
                    endforeach
                    error('Expected to find one of: ' + p)
                else
                    all_found_deps += xof_dep
                    conf.set10(all_optional_deps[k]['have'], true)
                endif
            else
                op_dep = dependency(
                    ok,
                    required: required,
                    version: op_ver,
                )

                if op_dep.found()
                    opt_found = true
                    all_found_deps += op_dep
                    conf.set10(all_optional_deps[k]['have'], true)
                endif
            endif
        endforeach
    endif
endforeach

# NLS
libintl = dependency('intl', required: false)
if libintl.found()
    all_found_deps += libintl
    conf.set10('HAVE_NLS', true)
endif

iconv = dependency('iconv')
if iconv.found()
    conf.set10('HAVE_ICONV', true)
    all_found_deps += iconv
endif

if cc.has_header_symbol('libcharset.h', 'locale_charset')
    conf.set10('HAVE_LIBCHARSET', true)
endif

# Hard-coded
non_configurable_ops = [
    'FMiniIconsSupported',
    'HAVE_XSM',
    'FVWM_COLORSET_PRIVATE',
]

foreach nco : non_configurable_ops
    conf.set10(nco, true)
endforeach

asciidoctor = find_program('asciidoctor', required: false)

mandoc_explain = 'Yes'
if get_option('mandoc') == true and not asciidoctor.found()
    mandoc_explain = 'No (asciidoctor not found)'
elif get_option('mandoc') == false
    mandoc_explain = 'No'
endif

htmldoc_explain = 'Yes'
if get_option('htmldoc') == true and not asciidoctor.found()
    htmldoc_explain = 'No (asciidoctor not found)'
elif get_option('htmldoc') == false
    htmldoc_explain = 'No'
endif

conf_template = '@0@/config_defines.h'.format(meson.current_source_dir())

conf_out = configure_file(
    output: 'configtemp.h',
    configuration: conf,
)

conf_cmd = '@0@/runme.sh'.format(meson.current_source_dir())
build_dir = '@0@/config.h'.format(meson.current_build_dir())
configure_file(
    output: 'config.h',
    command: [conf_cmd, conf_out, conf_template, build_dir],
)

libraries = []
lib_includes = include_directories('.')
fvwm_includes = include_directories('fvwm')
subdir('libs')

subdir('po')

subdir('fvwm')

fvwm3 = executable(
    meson.project_name(),
    fvwm_sources,
    include_directories: [lib_includes, fvwm_includes],
    dependencies: all_found_deps,
    link_with: libraries,
)

modules = {
    'FvwmAnimate': {'subdir': 'modules/FvwmAnimate'},
    'FvwmAuto': {'subdir': 'modules/FvwmAuto'},
    'FvwmBacker': {'subdir': 'modules/FvwmBacker'},
    'FvwmButtons': {'subdir': 'modules/FvwmButtons'},
    #FvwmConsole (TODO)
    'FvwmEvent': {'subdir': 'modules/FvwmEvent'},

}

module_sources = []
foreach m, _ : modules
    subdir(modules[m]['subdir'])
endforeach

summary(
    {
        'prefix': get_option('prefix'),
        'bindir': get_option('bindir'),
        'libdir': get_option('libdir'),
        'libexecdir': get_option('libexecdir'),
        'datadir': get_option('datadir'),
    },
    section: 'Directories',
)
summary(
    {
        'iconv': iconv,
    },
    section: 'Libraries',
)
summary(
    {
        'Module dir': conf.get('FVWM_MODULEDIR'),
        'Man pages': mandoc_explain,
        'HTML pages': htmldoc_explain,
    },
    section: 'Configuration',
)
