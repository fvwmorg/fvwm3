Installation Instructions
=========================

FVWM3 uses `meson` as its build tooling.

Dependencies
============

Docker
======

The `fvwm3` repository has a Dockerfile which can be used to build the
repository.  This is the same Docker image as used by Github Actions.

Manually
========

FVWM3 has the following dependencies.  Note that across different
distributions, the development package names will differ.  The names listed
below are examples to help you find the appropriately named package for the
system in use.

## Core dependencies

* libevent-dev (>= 2.0)
* libfontconfig-dev
* libfreetype6-dev
* libx11-dev
* libxext-dev
* libxft-dev
* libxkbcommon-dev
* libxrandr-dev (>= 1.5)
* libxrender-dev
* libxt-dev
* xtrans-dev

## Optional dependencies

* asciidoctor
* golang
* libfribidi-dev
* libncurses5-dev
* libpng-dev
* libreadline-dev
* librsvg-dev
* libsm-dev
* libxcursor-dev
* libxfixes-dev
* libxi-dev
* libxpm-dev
* sharutils

Generating documentation
========================

`fvwm3` won't compile documentation by default, so it's opt-in.

To generate `fvwm3`'s documentation:

1. Install `asciidoctor`
2. To generate manpages:  pass `-Dmandoc=true` to `meson`
3. To generate HTML docs: pass `-Dhtmldoc=true` to `meson`


Installing From Git or a Release Tarball
===================

Compiling `fvwm3` with meson involves the following command.  Note that this
is an example; the setup command can be passed various options, see the
`meson.options` file.

```
meson setup build && meson compile -C build && meson install -C build
```

However, a wrapper script, `build.sh` can be used as a convenience to running
manual meson commands.  See `./build.sh -h` for how to use it.
