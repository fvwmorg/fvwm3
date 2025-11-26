Installation Instructions
=========================

FVWM3 uses `meson` as its build tooling.  However, there is a wrapper script
`build` which should be used instead.  This document will use this throughout,
although if using `meson` directly, refer to its documentation.

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
2. To generate manpages:  pass `-m` to `build`
3. To generate HTML docs: pass `-w` to `build`

To compile both at once, pass `-d` to `build` (which implies `-m` and `-d`)

Installing From Git or a Release Tarball
===================

Compiling `fvwm3` is best used with the `./build` wrapper script.

```
./build -fdi
```

This will (re-)build, compile both man and HTML documentation, and install.

It is best to see the output from `./build -h` to understand its options,
but some common usages are:

```
# Build fvwm3 will both man and html pages
$ ./build -d

# Build and install
$ ./build -i

# Force (re-)build, including docs and install
$ ./build -fdi

# Enable specific options (see `meson.options`), compile docs and install
$ ./build -Dgolang=disabled -Dbidi=disabled -di
```
