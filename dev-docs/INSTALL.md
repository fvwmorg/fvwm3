Installation Instructions
=========================

FVWM3 uses automake and autotools as its build process.

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

To generate `fvwm3`'s documentation:

1. Install `asciidoctor`
2. To generate manpages:  pass `--enable-mandoc` to `./configure`
3. To generate HTML docs: pass `--enable-htmldoc` to `./configure`

`fvwm3` won't compile documentation by default, so it's opt-in.

Installing From Git
===================

FVWM3 has a bootstrap script to generate `configure` and associated files.
Run the following command chain to generate the `configure` script and build
the project:

```
./autogen.sh && ./configure && make
```

Installing From Release Tarball
===============================

Release tarballs will come bundled with `./configure` already, hence:

```
./configure && make
```

As with most things, if the default options `./configure` chooses isn't
appropriate for your needs, see `./configure --help` for appropriate options.
