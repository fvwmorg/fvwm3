project(
  'fvwm3',
  'c',
  version: run_command('./utils/fvwm-version-str.sh', check: false).stdout().strip(),
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
      '-Wno-implicit_int',
      '-fno-common',
      '-Wno-error=format-security',
  ],
  language: 'c',
)

cc = meson.get_compiler('c')
host = host_machine.system()

datadir = get_option('datadir')
sysconfdir = get_option('sysconfdir')
prefix = get_option('prefix')

syslibs = []

conf = configuration_data()

# Set static configuration.
conf.set_quoted('FVWM2RC', '.fvwm2rc')
conf.set_quoted('FVWM_CONFIG', 'config')
conf.set_quoted('FVWM_IMAGEPATH', '/usr/include/X11/bitmaps:/usr/include/X11/pixmaps')

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
    conf.set('HAVE_' + h.to_upper().underscorify(), true)
  endif
endforeach

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
  'socket',
  'strchr',
  'strdup',
  'strstr',
  'strtol',
  'sysconf',
  'uname',
  'waitpid',
]

foreach f : functions
  if cc.has_function(f, dependencies: syslibs)
    conf.set('HAVE_' + f.to_upper().underscorify(), true)
  endif
endforeach

# Dependencies.
x11 = dependency(
  'x11',
  required: true,
  not_found_message: 'X11 libraries or header files could not be found.',
)

xt = dependency(
  'xt',
  required: true,
  not_found_message: 'lixbt libraries or header files could not be found.',
)

sm = dependency(
  'sm',
  required: false,
)

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

if not sm.found() and get_option('sm') == true
  error('libsm requested, but the library cannot be found')
endif

conf.set('SESSION', sm.found() and get_option('sm') == true)

configure_file(output: 'config-new.h',
  configuration: conf)

summary({
  'prefix': get_option('prefix'),
  'bindir': get_option('bindir'),
  'libdir': get_option('libdir'),
  'datadir': get_option('datadir')
}, section: 'Directories')
summary({
  'Man pages': mandoc_explain,
  'HTML pages': htmldoc_explain,
}, section: 'Configuration')