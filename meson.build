project(
  'fvwm3',
  'c',

  version: '1.0.5',
  license: 'GPLv2',
  meson_version: '>=0.60.0',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'werror=false',
    ],
  )

add_project_arguments(
  [
    '-Wno-unused-parameter',
    '-Wno-missing-braces',
    '-Wno-unused-result',
    '-Wvla',
    '-Wundef',
    ],
  language: 'c',
  )

add_project_arguments('-D_GNU_SOURCE', language : 'c')
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')

cc = meson.get_compiler('c')

datadir = get_option('datadir')
sysconfdir = get_option('sysconfdir')
prefix = get_option('prefix')

version = '"@0@"'.format(meson.project_version())
git = find_program('git', native: true, required: false)
if git.found()
  git_commit = run_command([git, 'rev-parse', '--short', 'HEAD'], check: false)
        git_branch = run_command([git, 'rev-parse', '--abbrev-ref', 'HEAD'], check: false)
        if git_commit.returncode() == 0 and git_branch.returncode() == 0
          version = '"@0@-@1@ (" __DATE__ ", branch \'@2@\')"'.format(
            meson.project_version(),
            git_commit.stdout().strip(),
            git_branch.stdout().strip(),
            )
        endif
endif
add_project_arguments('-DVERSIONINFO=@0@'.format(version), language: 'c')

# Dependencies - REQUIRED
libevent = dependency('libevent')
libx11 = dependency('x11')
libxt = dependency('xt')
xft = dependency('xft')
xrandr = dependency('xrandr')
xrender = dependency('xrender')

# Dependencies - OPTIONAL
# See: meson_options.txt
asciidoctor = find_program(['asciidoctor'], required: get_option('asciidoctor'))
shar = find_program(['shar'], required: get_option('shar'))
go = find_program(['go'], required: get_option('go'))
fontconfig = dependency('fontconfig', required: get_option('fontconfig'))
freetype = dependency('freetype2', version: '>=2.12.1', required: get_option('freetype'))
fribidi = dependency('fribidi', required: get_option('fribidi'))
ncurses = dependency('ncurses', required: get_option('ncurses'))
libpng = dependency('libpng', required: get_option('libpng'))
readline = dependency('readline', required: get_option('readline'))
rsvg = dependency('librsvg-2.0', required: get_option('rsvg'))
sm = dependency('sm', required: get_option('sm'))
xcursor = dependency('xcursor', required: get_option('xcursor'))
xext = dependency('xext', required: get_option('xent'))
xi = dependency('xi', required: get_option('xi'))
xpm = dependency('xpm', required: get_option('xpm'))

#fvwm_includes = include_directories('fvwm', 'libs')
#subdir('fvwm')
#subdir('modules')

summary({
'libevent': libevent,
'libx11': libx11,
'libxt': libxt,
'xft': xft,
'xrandr': xrandr,
'xrender': xrender,

'asciidoctor': asciidoctor,
'go': go,
'shar': shar,
'fontconfig': fontconfig,
'freetype': freetype,
'fribidi': fribidi,
'ncurses': ncurses,
'libpng': libpng,
'readline': readline,
'rsvg': rsvg,
'sm': sm,
'xcursor': xcursor,
'xext': xext,
'xi': xi,
'xpm': xpm,
}, bool_yn: true)
