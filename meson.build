project(
    'fvwm3',
    'c',
    version: run_command(['cat', '.current-version'], check: true).stdout().strip(),
    license: 'GPL-2.0-or-later',
    meson_version: '>=1.5.1', # Required to configure perl files without eating \@s
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

# This script is used to clean up the release tarball when calling
# "meson dist".
meson.add_dist_script('meson-scripts/dist-tarball.sh')

cc = meson.get_compiler('c')

# Undefine _FOTIFY_SOURCE explicitly.  fvwm3 needs work to support this.
cc_flags = ['-U_FORTIFY_SOURCE']
foreach flag : cc_flags
    if cc.has_argument(flag)
        add_project_arguments(flag, language: 'c')
    endif
endforeach

host = host_machine.system() # This is the target system when cross compiling

m_dep = cc.find_library('m', required: false)
if m_dep.found()
    add_project_link_arguments('-lm', language: 'c')
endif

# Convenience helpers for things we might refer to a few times.
bindir = get_option('bindir')
datadir = get_option('datadir')
sysconfdir = get_option('sysconfdir')
prefix = get_option('prefix')
fvwm_datadir = join_paths(prefix, datadir, meson.project_name())
fvwm_moduledir = (
    prefix / get_option('libexecdir') / meson.project_name() / meson.project_version()
)
fvwm_vcs_versioninfo = run_command(
    './utils/fvwm-version-str.sh',
    check: false,
).stdout().strip()

syslibs = []

conf = configuration_data()

if host_machine.endian() == 'big'
    conf.set('WORDS_BIGENDIAN', true)
endif

if host_machine.system() == 'darwin'
    conf.set10('HOST_MACOS', true)
    add_project_arguments(['-Wno-nonportable-include-path'], language: 'c')
endif

# Set static configuration.
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('VERSIONINFO', fvwm_vcs_versioninfo)
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
conf.set_quoted('FVWM_DATADIR', fvwm_datadir)
conf.set_quoted('LOCALEDIR', prefix / get_option('localedir'))
conf.set_quoted('FVWM_CONFDIR', prefix / get_option('sysconfdir'))
conf.set('RETSIGTYPE', 'void')
conf.set('SIGNAL_RETURN', 'return')
conf.set('ICONV_ARG_CONST', '')
conf.set('fd_set_size_t', 'int')
conf.set('EXECUTABLE_EXTENSION', 'NULL')

# Enable FTMs
if host_machine.system() == 'linux'
    conf.set10('_DEFAULT_SOURCE', true) # usleep
endif

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
    'strings.h',
    'sys/select.h',
    'sys/stat.h',
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
functions = [
    'div',
    'getpwuid',
    'gettimeofday',
    'lstat',
    'memcpy',
    'memmove',
    'mkfifo',
    'putenv',
    'setpgid',
    'setpgrp',
    'setvbuf',
    'sigaction',
    'siginterrupt',
    'siglongjmp',
    'sigsetjmp',
    'socket',
    'strchr',
    'strdup',
    'strstr',
    'strtol',
    'sysconf',
    'intl',
    'uname',
    'wait3',
    'waitpid',
]

foreach f : functions
    if cc.has_function(f, dependencies: syslibs)
        val = 'HAVE_' + f.to_upper().underscorify()
        conf.set10(val, true)
    endif
endforeach

# SETPGRP_VOID is obsolete
# All current systems have a setpgrp whose signature conforms to Posix.
# We need to set this to use the correct implementation internally
# but we don't need to check for it.
if conf.has('HAVE_SETPGRP')
    conf.set10('SETPGRP_VOID', true)
endif

# Dependencies.
all_found_deps = []

# Required dependencies, we can easily iterate over these.
summary_depvals = {}
all_req_deps = ['fontconfig', 'ice', 'libevent', 'x11', 'xft', 'xrandr', 'xt', 'xtrans']
foreach rd : all_req_deps
    this_dep = dependency(rd, required: true)
    summary_depvals += {rd: this_dep}
    conf.set10('HAVE_' + rd.to_upper().underscorify(), true)
    all_found_deps += this_dep
endforeach

xext = dependency('xext', required: true)
all_found_deps += xext
summary_depvals += {'xext': xext}
conf.set10('SHAPE', true)

xkbcommon = dependency('xkbcommon', required: true)
all_found_deps += xkbcommon
summary_depvals += {'xkbcommon': xkbcommon}
conf.set10('HAVE_X11_XKBLIB_H', true)

# Python is a required dependency, we generate shebangs at buildtime
# This will error by default if there is no python interpreter found
py_mod = import('python')
if get_option('python') == ''
    # Just look for a python3 impl using standard logic, we weren't given a value
    # as an option
    py = py_mod.find_installation('python3')
else
    # Try and find python by provided version or path
    message('Trying to find python3: ' + get_option('python'))
    py = py_mod.find_installation(get_option('python'))
endif

# Perl is a required dependency, we generate shebangs at buildtime
perl = find_program('perl', required: true)

golang = find_program('go', required: get_option('golang'))
if golang.found()
    gover = run_command('go', 'version', check: true).stdout().split(' ')
    gover = gover[2].replace('go', '')
    if gover.version_compare('<1.20.0')
        error(
            '''A version of Go >= 1.20.0 is required to use the FVWM3 meson build Go modules.
               Either disable Go for this build (`-Dgolang=disabled`) or upgrade Go to a version >= 1.20.0.''',
        )
    endif
else
    # the readline dependency is only required for FvwmConsole so let's only check for it if we're not building FvwmPrompt
    readline = dependency('readline', required: get_option('readline'))
    if readline.found()
        all_found_deps += readline
        conf.set10('HAVE_READLINE', true)
        # Check for append_history to determine if we have GNU readline
        if cc.has_function('append_history', dependencies: readline)
            conf.set10('HAVE_GNU_READLINE', true)
        endif
    endif
endif

# Sed is used for substitution in several configure_file calls
sed = find_program('sed', required: true)

# Optional dependencies
# Tip: 'Feature' types always report 'not found' if disabled

freetype = dependency('freetype2', required: get_option('freetype'))
if freetype.found()
    all_found_deps += freetype
    conf.set10('HAVE_FREETYPE', true)
endif

fribidi = dependency(
    'fribidi',
    version: '>=0.19.2',
    required: get_option('bidi'),
)
if fribidi.found()
    all_found_deps += fribidi
    conf.set10('HAVE_BIDI', true)
endif

iconv_warning = false

iconv = dependency('iconv', required: get_option('iconv'))
if iconv.found()
    conf.set10('HAVE_ICONV', true)
    all_found_deps += iconv

    # Check if we have nl_langinfo function (typically glibc)
    if cc.has_function('nl_langinfo')
        conf.set10('HAVE_CODESET', true)
    endif

    libcharset_code = '''
    #include <libcharset.h>

    int main(void) {
        const char *charset = locale_charset();
        return 0;
    }
    '''

    if cc.links(libcharset_code, dependencies: iconv, name: 'libcharset check')
        conf.set10('HAVE_LIBCHARSET', true)
    elif cc.has_header_symbol('libcharset.h', 'locale_charset')
        # If header is present but linking fails, it might be a conflict
        # we'll warn after the summary so that this isn't lost in configure noise
        iconv_warning = true
    endif
endif

libintl = dependency('intl', required: get_option('nls'))
if libintl.found()
    all_found_deps += libintl
    conf.set10('HAVE_NLS', true)
endif

libpng = dependency('libpng', version: '>=1.2', required: get_option('png'))
if libpng.found()
    all_found_deps += libpng
    conf.set10('HAVE_PNG', true)
else
    # We only check for libpng, no automagic
    libpng_check = dependency('libpng', required: false)
    if libpng_check.found()
        png_msg = '''libpng found, and -Dpng=disabled was given.
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

librsvg = dependency(
    'librsvg-2.0',
    version: '>=2.13.92',
    required: get_option('svg'),
)
if librsvg.found()
    all_found_deps += librsvg
    conf.set10('HAVE_RSVG', true)
    # We need at least one of these dependencies
    svg_backends = ['cairo', 'cairo-svg', 'libsvg-cairo']
    svg_deps = []
    foreach d : svg_backends
        # By treating these as features we can let automagic find one or more backends
        # or we can explicitly set one or more backends as required, or disable ones we don't want to use.
        svg_dep = dependency(d, required: get_option(d))
        if svg_dep.found()
            svg_deps += svg_dep
            summary_depvals += {d: svg_dep}
            conf.set10('HAVE_' + d.to_upper().underscorify(), true)
            break
        endif
    endforeach
    if svg_deps.length() == 0
        # If everything is set to auto (or the user turns all the backends off) we need to explicitly fail here.
        error(
            'librsvg found but also require one of: '
            + ', '.join(svg_backends)
            + ' to build SVG support.',
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

xcursor = dependency('xcursor', required: get_option('xcursor'))
if xcursor.found()
    all_found_deps += xcursor
    conf.set10('HAVE_XCURSOR', true)
endif

xfixes = dependency('xfixes', required: get_option('xfixes'))
if xfixes.found()
    all_found_deps += xfixes
    conf.set10('HAVE_XFIXES', true)
endif

xpm = dependency('xpm', required: get_option('xpm'))
if xpm.found()
    all_found_deps += xpm
    conf.set10('XPM', true)
endif

xrender = dependency('xrender', required: get_option('xrender'))
if xrender.found()
    all_found_deps += xrender
    conf.set10('HAVE_XRENDER', true)
endif

# Hard-coded
non_configurable_ops = [
    'FMiniIconsSupported',
    'FORK_CREATES_CHILD',
    'HAVE_SAFETY_MKSTEMP', #  safer since 2008; no real need to check for this on a modern system.
    'HAVE_X11_FD',
    'HAVE_XOUTPUT_METHOD',
    'HAVE_XSHM',
]

foreach nco : non_configurable_ops
    conf.set10(nco, true)
endforeach

build_docs = false
if get_option('htmldoc') or get_option('mandoc')
    build_docs = true
    asciidoc = find_program('asciidoctor', required: build_docs)
    grep = find_program('grep')
    subdir('doc')
endif

conf_template = '@0@/config_defines.h'.format(meson.current_source_dir())

conf_out = configure_file(
    output: 'configtemp.h',
    configuration: conf,
)

conf_cmd = '@0@/meson-scripts/conf-cmd.sh'.format(meson.current_source_dir())
build_dir = '@0@/config.h'.format(meson.current_build_dir())
configure_file(
    output: 'config.h',
    command: [conf_cmd, conf_out, conf_template, build_dir],
)

# Configured files inherit the permissions of the template,
# so we need to set the permissions explicitly for executable scripts.
install_mask_755 = 'rwxr-xr-x'

includedirs = []
# To be consistent with the autotools build, we need to add /usr/local/include
# to the include path on *BSD.
if host in ['freebsd', 'openbsd']
    usrlocal_includes = include_directories('/usr/local/include')
    includedirs += usrlocal_includes
endif
lib_includes = include_directories('.')
fvwm_includes = include_directories('fvwm')

includedirs += lib_includes
includedirs += fvwm_includes

# These are used to configure files in 'bin', 'perllib', and other misc files.
# while we use terms like 'bindir' here, these don't (quite) map to meson;
# most things in fvwm3 want full paths. Autotools would use variables in the
# output, but we can construct full paths here instead.

# This could be tidied up, but we just want to be consistent with
# autotools for now.

file_config = configuration_data()
file_config.set('bindir', prefix / bindir)
file_config.set('datadir', prefix / datadir) # We could probably simplify this and the below into one config...
file_config.set('datarootdir', prefix / datadir)
file_config.set('FVWM_DATADIR', fvwm_datadir)
file_config.set(
    'FVWM_PERLLIBDIR',
    fvwm_datadir / 'perllib',
)
file_config.set('PERL', perl.full_path())
file_config.set('prefix', prefix)
file_config.set('PYTHON', py.full_path())
file_config.set('SED', sed.full_path())
file_config.set('VERSION', meson.project_version())
file_config.set('VERSIONINFO', fvwm_vcs_versioninfo)

subdir('po')
subdir('libs')
subdir('fvwm')
subdir('default-config')
subdir('perllib')

fvwm3 = executable(
    meson.project_name(),
    fvwm_sources,
    include_directories: includedirs,
    dependencies: all_found_deps,
    link_with: libfvwm3,
    install: true,
)

# Core modules
modules = {
    'FvwmAnimate': {'subdir': 'modules/FvwmAnimate'},
    'FvwmAuto': {'subdir': 'modules/FvwmAuto'},
    'FvwmBacker': {'subdir': 'modules/FvwmBacker'},
    'FvwmButtons': {'subdir': 'modules/FvwmButtons'},
    'FvwmEvent': {'subdir': 'modules/FvwmEvent'},
    'FvwmForm': {'subdir': 'modules/FvwmForm'},
    'FvwmIconMan': {'subdir': 'modules/FvwmIconMan'},
    'FvwmIdent': {'subdir': 'modules/FvwmIdent'},
    'FvwmMFL': {'subdir': 'modules/FvwmMFL'},
    'FvwmPager': {'subdir': 'modules/FvwmPager'},
    'FvwmPerl': {'subdir': 'modules/FvwmPerl'},
    'FvwmRearrange': {'subdir': 'modules/FvwmRearrange'},
    'FvwmScript': {'subdir': 'modules/FvwmScript'},
}

if not golang.found()
    modules += {'FvwmConsole': {'subdir': 'modules/FvwmConsole'}}
endif

foreach m, _ : modules
    subdir(modules[m]['subdir'])
endforeach

# FvwmPrompt and FvwmCommand
subdir('bin')

# Contrib
# Provide a freedesktop-compilant, minimal xsession.
configure_file(
    input: 'contrib/xsession/fvwm3.desktop.in',
    output: 'fvwm3.desktop',
    configuration: file_config,
    install_dir: datadir / 'xsessions',
    install: true,
)

summary_docdir = get_option('datadir') / 'doc' / meson.project_name()
if get_option('docdir') != ''
    summary_docdir = get_option('docdir')
endif

summary(
    {
        'prefix': get_option('prefix'),
        'bindir': get_option('bindir'),
        'datadir': get_option('datadir'),
        'docdir': summary_docdir,
        'libdir': get_option('libdir'),
        'libexecdir': get_option('libexecdir'),
        'Module dir': get_option('libexecdir') / meson.project_name() / meson.project_version(),
    },
    section: 'Directories',
)

environment_summary = {
    'Build': build_machine.system(),
    'Build CPU Family': build_machine.cpu_family(),
    'Host CPU Family': host_machine.cpu_family(),
    'Cross-compiling': meson.is_cross_build(),
    'Build Endianness': build_machine.endian(),
    'Host Endianness': host_machine.endian(),
    'Target': host,
    'VCS Version': fvwm_vcs_versioninfo,
    'C Compiler': cc.get_id(),
    'C Compiler Version': cc.version(),
    'Go Version': golang.found() ? gover : 'N/A',
    'Linker': cc.get_linker_id(),
    'Python Path': py.full_path(),
    'Python Version': py.version(),
    'Perl': perl,
    'Perl Version': run_command(
        perl,
        '-e', 'print "$^V\\n";',
        check: true,
    ).stdout().strip('v'),
}

if build_docs
    environment_summary += {
        'asciidoctor': asciidoc.version(),
    }
endif

summary(
    environment_summary,
    section: 'Environment',
)

summary(
    {
        'HTML pages': get_option('htmldoc'),
        'Man pages': get_option('mandoc'),
    },
    bool_yn: true,
    section: 'Documentation',
)

featurevals = {
    'BiDi': fribidi.found() ? fribidi : false,
    'FreeType': freetype.found() ? freetype : false,
    'Go Modules': golang.found(),
    'iconv': iconv.found(),
    'NLS': libintl.found(),
    'PNG support': libpng.found() ? libpng : false,
    'Session Management': sm.found() ? sm : false,
    'SVG support': librsvg.found() ? librsvg : false,
    'Xcursor': xcursor.found() ? xcursor : false,
    'XFixes': xfixes.found() ? xfixes : false,
    'XPM support': xpm.found() ? xpm : false,
    'XRender': xrender.found() ? xrender : false,
}

if not golang.found()
    featurevals += {'Readline': readline.found()}
endif

summary(
    featurevals,
    bool_yn: true,
    section: 'Features',
)

summary(
    summary_depvals,
    bool_yn: true,
    section: 'Dependencies',
)

if iconv_warning
    warning(
        '''Found libcharset.h header, but couldn't link against locale_charset() function.
        This typically happens when GNU libiconv conflicts with the iconv built into libc.
        GNU libc systems shouldn't have GNU libiconv installed as they already provide iconv.
        This is a bug in your distribution's packaging.
        Please report this to your distribution's developers.
        '''
    )
endif
