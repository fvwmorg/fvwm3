Installation Instructions
=========================

FVWM uses automake and friends as its build process. 

Installing From Git
===================

FVWM has a bootstrap script to generate `configure` and associated files.
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
