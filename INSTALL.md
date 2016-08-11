Installation Instructions
=========================

FVWM uses automake and friends as its build process. 

Installing From Git
===================

FVWM has a bootstrap script to generate `configure` and associated files.
Run:

```
./autogen.sh
```

This will also call `./configure` after it has been generated.  Then run:

```
make
```

Installing From Release Tarball
===============================

Release tarballs will come bundled with `./configure` already, hence:

```
./configure && make
```

As with most things, if the default options `./configure` chooses isn't
appropriate for your needs, see `./configure --help` for appropriate options.
