# Build Systems

FVWM3 supports both autotools and Meson as its build systems.

Why?

- Autotools is the traditional build system for FVWM, and is still used by non-Linux systems.
  It is retained for compatibility with systems that do not support Meson.
- Meson is a modern build system that is faster and easier to use than autotools.

When adding new files to the source tree, or changing dependencies or configuration options
you will need to update both build systems to include the new files.

This is done by adding the new files to the appropriate build system file (`meson.build`, `Makefile.am`),
or for configuration changes, by updating the appropriate configuration file (`configure.ac`, `meson.build`, `meson.options`).

## Meson

## Options: Booleans vs Features

Meson has a selection of options that can be set by the user when configuring the build.

Often, these are used to enable or disable features (i.e. are set to `true` or `false`).

These can be directly queried for truthiness in the `meson.build` file.

```meson.build
if get_option('bar')
  # do something
endif
```

There also exists a `features` system in Meson, which is a more advanced way to handle options.
Features are defined in the `meson.options` file, and can be set to `auto`, `enabled`, or `disabled`.

These are envisioned to be used for dependencies, where the developer can use a feature to
set the 'required' property of a dependency.

  - `enabled` is the same as passing `required : true`.
  - `auto` is the same as passing `required : false`.
  - `disabled` do not look for the dependency and always return 'not-found'.

```meson.build
dep = dependency('foo', required : get_option('bar'))
```

See the [Reference Manual](https://mesonbuild.com/Reference-manual_returned_feature.html) for more information
on the feature system and the more complex ways it can be used.

When building the software, the user can specify which features to enable or disable, like with booleans,
however the `auto_features` option controls the default value of all features set to `auto`. By default it
is set to `auto`, but can be set (by the user) to `enabled` or `disabled` to
override the default value of all features set to `auto`.

The user can still override `auto_features` by specifying the feature directly.

```bash
user@host:~/$ meson build -Dauto_features=enabled -Dfeature_name=disabled
```

### What's the difference?

You can do a lot of the same things with both options and features, but features are more
flexible and can be used to control multiple options at once, or toggled within the build system
without needing to change the user's configuration.

Features can also take advantage of convenient helpers like `enable_if` and `disable_if` to avoid
tons of boilerplate code.

```meson.build
dep = dependency('foo', required : get_option('bar'))
if dep.found()
  foobar = executable('foobar', 'foobar.c', dependencies : dep)
endif

if get_option('baz').allowed()
  # feature is enabled or 'auto'
endif

cool_feature = get_option('qux') \
  .disable_if(host_machine.system() == 'darwin', error_message : 'this cool fvwm feature not supported on MacOS')
dep_cool_feature = dependency('cool_feature', required: cool_feature)

build_docs = get_option(
    'docs',
).enable_if(get_option('htmldoc')).enable_if(get_option('mandoc'))
asciidoc = find_program('asciidoctor', required: build_docs)

if asciidoc.found()
    subdir('doc')
endif
```

Using features also enables the user to easily control all
of the feature options at once (a distribution packager may
disable all `auto` features and selectively enable only a few, for example).

TL;DR:

Use a boolean option:

  - When the option has only two states: enabled or disabled (e.g., enable_debugging).
  - When the option simply controls the presence or absence of a functionality (e.g., build_tests).
  - For simple on/off toggles that don't require additional configuration.

Use a Meson Feature:

  - When the option triggers additional configuration or dependency logic based on its state (e.g., a database feature with choices like sqlite, mysql, or none).
  - To provide informative messages or warnings depending on the chosen feature state.

# Distribution Packagers

You should probably prefer Meson on systems that support it, as it is faster and more modern than autotools.

The Ninja backend is faster than make, and Meson's configuration is straightforward.
Meson also has better support for cross-compilation, and the input files are more readable / auditable.

If you are packaging FVWM3 for a system that does not support Meson, you can use Muon C99 implementation,
however this receives less testing and is not recommended for general use.
