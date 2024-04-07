project(
    'fvwm3',
    'c',
    version: run_command(
        './utils/fvwm-version-str.sh',
        check: false,
    ).stdout().strip(),
    license: 'GPL',
    meson_version: '>=1.1.0', #feature_option.enable_if()
    default_options: [
        'c_std=c11',
        'warning_level=2',
        'werror=false',
    ],
)

add_project_arguments(
    [
        '-fno-common',
        '-Wno-error=format-security',
        '-Wno-sign-compare',
        '-Wno-unused-parameter',
        '-Wno-extra',
    ],
    language: 'c',
)

cc = meson.get_compiler('c')
host = host_machine.system() # This is the target system when cross compiling

m_dep = cc.find_library('m', required: false)
if m_dep.found()
    add_project_link_arguments('-lm', language: 'c')
endif

bindir = get_option('bindir')
datadir = get_option('datadir')
sysconfdir = get_option('sysconfdir')
prefix = get_option('prefix')
fvwm_moduledir = (
    get_option('libexecdir') + '/' + meson.project_name() + '/' + meson.project_version()
)

syslibs = []

conf = configuration_data()

if host_machine.endian() == 'big'
    conf.set('WORDS_BIGENDIAN', true)
endif

# Set static configuration.
conf.set_quoted('VERSION', '1.1.1')
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
    fvwm_moduledir,
)
conf.set_quoted('FVWM_DATADIR', datadir + '/' + meson.project_name())
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

if cc.has_header_symbol('libcharset.h', 'locale_charset')
    conf.set10('HAVE_LIBCHARSET', true)
endif

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

# Required dependencies, we can easily iterate over these.

all_req_deps = ['x11', 'xt', 'xrandr', 'libevent', 'ice']
foreach rd : all_req_deps
    this_dep = dependency(rd, required: true)
    conf.set10('HAVE_' + rd.to_upper().underscorify(), true)
    all_found_deps += this_dep
endforeach

# Python is a required dependency, we generate shebangs at buildtime
# This will error by default if there is no python interpreter found
py_mod = import('python')
py = py_mod.find_installation('python3')

# Perl is a required dependency, we generate shebangs at buildtime
perl = find_program('perl', required: true)

golang = find_program('go', required: get_option('golang'))

# Optional dependencies
# First we check for things specified by the user (i.e. no automagic)
# Then we check for automagic dependencies.
# Tip: 'Feature' types will always report 'not found' if disabled

fribidi = dependency(
    'fribidi',
    version: '>=0.19.2',
    required: get_option('bidi'),
)
if fribidi.found()
    all_found_deps += fribidi
    conf.set10('HAVE_FRIBIDI', true)
endif

iconv = dependency('iconv', required: get_option('iconv'))
if iconv.found()
    conf.set10('HAVE_ICONV', true)
    all_found_deps += iconv
endif

libintl = dependency('intl', required: get_option('nls'))
if libintl.found()
    all_found_deps += libintl
    conf.set10('HAVE_NLS', true)
endif

if get_option('png')
    libpng = dependency('libpng', version: '>=1.2', required: true)
    all_found_deps += libpng
    conf.set10('HAVE_PNG', true)
else
    # We only check for libpng, no automagic
    libpng = dependency('libpng', required: false)
    if libpng.found()
        png_msg = '''libpng found, and -Dpng=false was given.
        It is recommended that libpng be enabled so that PNG icons can
        be rendered by FVWM, which is expected by the default configuration
        as well as other third-party programs which generate menus,
        among others.

        If it is critical that FVWM does not link against libpng
        (perhaps due to size limitations of the binary),
        continue down this path.
        '''
    else
        png_msg = ''' Disabled PNG support means that the default
            configuration shipped with FVWM will be without icon
            support.  Be sure this is what you mean to do.
            '''
    endif
    warning(png_msg)
endif

if get_option('rsvg')
    librsvg = dependency('librsvg-2.0', version: '>=2.13.92', required: true)
    all_found_deps += librsvg
    conf.set10('HAVE_RSVG', true)
    # We need at least one of these dependencies
    svg_opt_deps = ['cairo', 'cairo-svg', 'libsvg-cairo']
    svg_deps = []
    foreach d : svg_opt_deps
        svg_dep = dependency(d, required: false)
        if svg_dep.found()
            svg_deps += svg_dep
            conf.set10('HAVE_' + d.to_upper().underscorify(), true)
            break
        endif
    endforeach
    if svg_deps.length() == 0
        error(
            'librsvg found but also require one of: ' + svg_opt_deps.join(' '),
        )
    else
        all_found_deps += svg_deps
    endif
endif

sm = dependency('sm', required: get_option('sm'))
if sm.found()
    all_found_deps += sm
    conf.set10('SESSION', true)
endif

if get_option('xent')
    xext = dependency('xext', required: true)
    all_found_deps += xext
    conf.set10('HAVE_SHAPE', true)
endif

# Automagic optional dependencies
# TODO: Automagic dependencies are a nightmare for
# downstream packagers who need to know what is
# and is not enabled for a package at build-time
# not what was randomly also found on their system.

# We should either make these required or hide them behind
# a feature flag.

freetype = dependency('freetype2', required: false)
if freetype.found()
    all_found_deps += freetype
    conf.set10('HAVE_XFT', true)
    freetype_opt_deps = ['xft', 'fontconfig']
    freetype_deps = []
    foreach d : freetype_opt_deps
        # These deps were previously listed as 'optional', but with 'required: true'
        freetype_dep = dependency(d, required: false)
        if freetype_dep.found()
            freetype_deps += freetype_dep
            conf.set10('HAVE_' + d.to_upper().underscorify(), true)
        endif
    endforeach
    if freetype_deps.length() > 0
        all_found_deps += freetype_deps
    endif
endif

xkbcommon = dependency('xkbcommon', required: false)
if xkbcommon.found()
    all_found_deps += xkbcommon
    conf.set10('HAVE_X11_XKBLIB_H', true)
endif

xpm = dependency('xpm', required: false)
if xpm.found()
    all_found_deps += xpm
    conf.set10('HAVE_XPM', true)
endif

automagic_opt_deps = ['readline', 'xcursor', 'xrender']
foreach ad : automagic_opt_deps
    this_dep = dependency(ad, required: true)
    conf.set10('HAVE_' + ad.to_upper().underscorify(), true)
    all_found_deps += this_dep
endforeach

# Hard-coded
non_configurable_ops = [
    'FMiniIconsSupported',
    'HAVE_XSM',
    'FVWM_COLORSET_PRIVATE',
]

foreach nco : non_configurable_ops
    conf.set10(nco, true)
endforeach

build_docs = get_option(
    'docs',
).enable_if(
    get_option('htmldoc') or get_option('mandoc'),
    error_message: 'Docs is diabled but at least one of htmldoc, mandoc is enabled',
).disable_if(
    (not get_option('htmldoc') and not get_option('mandoc')),
    error_message: 'Docs can\'t be enabled without at least one of: htmldoc, mandoc',
)

asciidoc = find_program('asciidoctor', required: build_docs)

if asciidoc.found()
    subdir('doc')
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
# Configured files inherit the permissions of the template,
# so we need to set the permissions explicitly for executable scripts.
install_mask_755 = 'rwxr-xr-x'
lib_includes = include_directories('.')
fvwm_includes = include_directories('fvwm')

subdir('libs')
subdir('po')
subdir('fvwm')
subdir('default-config')

fvwm3 = executable(
    meson.project_name(),
    fvwm_sources,
    include_directories: [lib_includes, fvwm_includes],
    dependencies: all_found_deps,
    link_with: libraries,
)

# Core modules
modules = {
    'FvwmAnimate': {'subdir': 'modules/FvwmAnimate'},
    'FvwmAuto': {'subdir': 'modules/FvwmAuto'},
    'FvwmBacker': {'subdir': 'modules/FvwmBacker'},
    'FvwmButtons': {'subdir': 'modules/FvwmButtons'},
    'FvwmEvent': {'subdir': 'modules/FvwmEvent'},
    'FvwmForm': {'subdir': 'modules/FvwmForm'},
}

if not golang.found()
    modules += {'FvwmConsole': {'subdir': 'modules/FvwmConsole'}}
endif

# FvwmPrompt and FvwmCommand
subdir('bin')

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
        'Module dir': conf.get_unquoted('FVWM_MODULEDIR'),
    },
    section: 'Directories',
)
summary(
    {
        'Build': build_machine.system(),
        'CPU Family': build_machine.cpu_family(),
        'Endianness': build_machine.endian(),
        'C Compiler': cc.get_id(),
        'C Compiler Version': cc.version(),
        'Linker': cc.get_linker_id(),
        'Python Path': py.full_path(),
        'Python Version': py.version(),
        'Perl': perl,
        'asciidoctor': asciidoc.found() ? asciidoc.full_path() : 'No',
        'asciidoctor version': asciidoc.found() ? asciidoc.version() : 'N/A',

    },
    section: 'Environment',
)
summary(
    {
        'Target': host,
        'CPU Family': host_machine.cpu_family(),
        'Endianness': host_machine.endian(),
        'Cross-compiling': meson.is_cross_build(),
        'bidi': get_option('bidi').enabled() ? fribidi : 'No',
        'iconv': get_option('iconv').enabled() ? iconv : 'No',
        'Go support': golang.found() ? golang : 'No',
        'Build Documentation': build_docs.enabled() ? 'Yes' : 'No',
        'HTML pages': get_option('htmldoc'),
        'Man pages': get_option('mandoc'),
        'NLS': libintl.found() ? libintl : 'No',
        'Session Management': sm.found() ? sm : 'No',
        'Shaped Windows': get_option('xent') ? xext : 'No',
        'PNG support': get_option('png') ? libpng : 'No',
        'SVG support': get_option('rsvg') ? librsvg : 'No',
    },
    section: 'Configuration',
)
