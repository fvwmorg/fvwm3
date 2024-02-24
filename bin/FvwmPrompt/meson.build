# Fvwmprompt is written in golang. Unfortunately there is still
# no automagic in meson to handle golang projects. So we have to
# do it manually.

# Extract all of the go sources from the current directory
r = run_command('find-go-sources.sh', meson.current_source_dir(), check: true)
fvwmprompt_files = r.stdout().strip().split()
fvwmprompt_files += 'go.mod'
fvwmprompt_files += 'go.sum'

fvwmprompt_build = meson.current_source_dir()
fvwmprompt_output = meson.current_build_dir() + '/FvwmPrompt'

fvwmprompt = custom_target(
    'FvwmPrompt',
    output: 'FvwmPrompt',
    input: fvwmprompt_files,
    command: [
        golang,
        'build',
        '-C', fvwmprompt_build,
        '-v',
        '-mod=vendor',
        '-o', fvwmprompt_output,
    ],
    install: true,
    install_dir: bindir,
)
