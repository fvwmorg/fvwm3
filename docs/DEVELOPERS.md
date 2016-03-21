Developing for FVWM
===================

This document aims to help the developer with the expectations when dealing
with the FVWM source code.

The FVWM source conforms to the [Linux kernel style
guide](https://www.kernel.org/doc/Documentation/CodingStyle).

Coventions
==========

The following tries to list all the conventions that the fvwm developers
adhere to, either by consensus through discussion, common practice or unspoken
agreement.  It is hopefully useful for the fvwm development newbie.

Programming Languages
--------------------

 The following programming languages are allowed:

- ANSI C
- Perl
- Portable /bin/sh scripts for examples.

New Code Files
--------------

- There are templates for new code files in the fvwm directory.  Try to always
  use them as they provide a clean structure of the header and code files.
  Please honour the section titles.  For example, put all static functions
  (and only static functions) under the "local functions" section.

- All .c files *must* have

```
#include "config.h"
```

as the first non-comment line.  Otherwise the settings made by the configure
script may not be used.  This can cause random problems.

File Names
----------

- The names of the code files in the fvwm directory are in lower case.
- Files in the libs directory may begin with a capital 'F'.  This letter is
  reserved for wrapper files for third party libraries or modules.  For
  example, FShape is an abstraction of the XShape X server extension and FBidi
  is a wrapper for the fribidi library.  Do not use the 'F' for other
  purposes.

Copyright Notices
-----------------

- A copy of the GPL should be at the beginning of all code files (.c) and
  scripts, but not at the beginning of header files (.h).

Maintaining Man Pages
---------------------

- Every feature must be described with all options in the man page.

Creating a release
==================

Before deciding to make a new release, please check with the `fvwm-workers`
mailing list that this is the right time to do so.  This will give adequate
warning for other developers to give status updates about any in-flight
development that's happening which might impact a potential release.

Make sure you have all optional libraries installed.

0. `git checkout master && git pull` 
1. Change the dates in configure.ac and fill in the release dates.
2. Verify that the version variable at the very beginning of
   configure.ac has the value of the going to be released version
   and set `ISRELEASED` to `yes`.
3. Commit the results:  `git commit -a`
4. Run: `./utils/configure_dev.sh && make clean` to get the tree into a clean
   slate.  Because this is a release, the source needs compiling.  To do
   that, run:

   ```
    make CFLAGS="-g -O2 -Wall -Wpointer-arith -fno-strict-aliasing -Werror"
   ```

    Fix all warnings and problems, commit the changes and repeat the previous
    command until no more warnings occur.
5. Build and test the release tarballs:

   Run: `make distcheck2`

   If that succeeds, check for `fvwm-x.y.z.tar.gz` in the current working
   directory.  This is the release tarball which will be uploaded to Github.
   Unpack it to a temporary directory and build it; check the version as well,
   via: `./fvwm --version`.
6. Tag the release: `git tag -a` 
7. Push the tag out: `git push --tags`
8. Upload the `fvwm-x.y.z.tar.gz` tarball to Github.
9. Increase the version number in `configure.ac` and set `ISRELEASED` to `no`
10. Commit that, and push it out.

Updating fvwm-web
=================

TBD
