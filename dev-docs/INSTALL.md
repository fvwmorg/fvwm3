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

FVWM3 has the following dependencies.  It's recommended to install all
dependencies, core or optional.

## Core dependencies

* libbson-dev
* libevent-dev (>= 2.0)
* libx11-dev
* libxrandr-dev (>= 1.5)
* libxrender-dev
* libxt-dev

## Optional dependencies

* libfontconfig-dev
* libfreetype6-dev
* libfribidi-dev
* libncurses5-dev
* libpng-dev
* libreadline-dev
* libsm-dev
* libx11-dev
* libxcursor-dev
* libxext-dev
* libxft-dev
* libxi-dev
* libxpm-dev
* sharutils
* xsltproc

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
