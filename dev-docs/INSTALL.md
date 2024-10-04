Installation Instructions
=========================

FVWM3 prefers `meson` as its build tool chain, but provides `autotools` as a
legacy fallback on older systems.

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
* libxrandr-dev (>= 1.5)
* libxrender-dev
* libxt-dev

## Optional dependencies

* asciidoctor
* libfribidi-dev
* libncurses5-dev
* libpng-dev
* libreadline-dev
* librsvg-dev
* libsm-dev
* libxcursor-dev
* libxi-dev
* libxpm-dev
* sharutils

Generating documentation
========================

`fvwm3` won't compile documentation by default, so it's opt-in.

To generate `fvwm3`'s documentation:

1. Install `asciidoctor`
2. To generate manpages:  pass `-Dhtmldoc=true` to `meson`
3. To generate HTML docs: pass `-Dmandoc=true` to `meson`


Installing From Git
===================

## Build Systems

`fvwm3` has traditionally been using autotools.  However, this is now
deprecated in favour of `meson`.  It is suggested that all systems which
support `meson` use this instead as it is now the preferred build system to
use.

The `autotools` build system remains to provide legacy support but is not
going to see any updates to it.

### Autotools

```
./autogen.sh && ./configure && make && sudo make install
```

### Meson

```
meson setup build && ninja -C build && ninja -C build install
```

Installing From Release Tarball
===============================

## Autotools

Release tarballs will come bundled with `./configure` already, hence:

```
./configure && make && sudo make install
```

## Meson

```
meson setup build && ninja -C build && meson install -C build
```