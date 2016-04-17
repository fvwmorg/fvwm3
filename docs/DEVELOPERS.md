<!--- IF THIS DOCUMENT IS UPDATED THEN ALSO UPDATE THE FVWM WEB REPO --->

Developing for FVWM
===================

This document aims to help the developer with the expectations when dealing
with the FVWM source code.

The FVWM source conforms to the [Linux kernel style
guide](https://www.kernel.org/doc/Documentation/CodingStyle).

Branch Workflows / Submitting Code Changes
==========================================

The main FVWM repository treats the `master` branch as stable, in that it's the
branch which has the most tested code on it, and the branch from which releases
are made.  Formal releases of FVWM are tagged, in the form `version-_x.y.z`.
Untagged code may well accumulate on `master`, which will go to form the next
release.

Other branches in the repository will reflect on-going development from core
fvwm-workers.   As such, these branches are often in a state of flux, and likely
to be rebased against other branches.  *NO* code should be based off topic
branches, unless explicitly agreed with other developers, who might need to
collaborate.

### Branch naming

Branch names are used to try and indicate the feature, and who is working on
them.  So for example, a topic-branch will be named as:

`initials/rough-description-of-branch`

For example:

`ta/fix-clang-warnings`

denotes that the branch is worked on by someone with the initials `TA` and that
the branch is about fixing warnings from Clang.

Sometimes, if more than one person is collaborating on a branch, the initials
prefix might not be needed.

### Submitting Pull-requests

External contributions are always welcomed and encouraged.  If you're thinking
of writing a new feature, it is worthwhile posting an email to the
`fvwm-workers` mailing list to discuss whether it's a good idea, and to check no
one else is working on that feature.

Those wishing to submit code/bug-fixes should:

* [Fork the FVWM-repository](https://github.com/fvwmorg/fvwm#fork-destination-box)
* Add the [FVWM-repo](https://github.com/fvwmorg/fvwm.git) as an upstream
  remote:
  * `git remote add fvwmorg https://github.com/fvwmorg/fvwm.git &&
    git fetch fvwmorg`
* Create a topic-branch to house your work;
* Rebase it against `fvwmorg/master`
* Push the latest changes to your fork;
* Open a pull-request

Once a pull-request is opened, an email is sent to the `fvwm-workers` list so we
can take a look at it.

Alternatively, if pull-requests are not an option, then `git-send-email` can be
used, sending the relevant patchsets to the `fvwm-workers` mailing list.

### Protected branches and the use of Travis-CI

Pull-requests made will result in the use of Travis-CI being run against the
branch.  This builds the copy of the pushed code in a Ubuntu environment, with
all the additional libraries FVWM could use, loaded in.  Builds are made against
`gcc` and `clang`, because both those compiler cover slightly different angles
with respect to compiling.  All warnings are treated as errors, and if a build
does not succeeded, ensure the code is fixed, and pushed back out on the same
branch.  Rebasing is recommended; Travis-CI and Github handle this just fine.

The FVWM repository also treats the `master` branch as protected.  This is a
[GitHub feature](https://help.github.com/articles/about-protected-branches/)
which means the `master` branch in this case cannot have changes merged into it
until Travis-CI has verified the builds do not fail.

This has benefit since not every developer will be using the same operating
systems (Linux versus BSD for instance), and that `master` is meant to try and
be as release-worthy as can be.

**NOTE**:  At present, this does not stop core fvwm-workers from force-pushing
onto `master`.  This might be useful in some cases, but isn't recommended.

Conventions
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

**NOTE:  as `master` is a protected branch, changes made to files during the
release phase must be done on a separate branch, and not on master directly,
as pushes to this branch are not allowed until checks have been done on it.
This means the end result of the release-phase must have these changes issued
as a pull-request against `master`.**

0. `git checkout master && git pull && git checkout -b release/x.y.z`
   **Where: `x.y.z` will be the next release**.
1. Change the dates in configure.ac and fill in the release dates.
2. Set `ISRELEASED` to `"yes"`.
3. Commit the results.
4. Run: `./autogen.sh && make clean` to get the tree into a clean
   slate.  Because this is a release, the source needs compiling.  To do
   that, run:

   ```
    make CFLAGS="-g -O2 -Wall -Wpointer-arith -fno-strict-aliasing -Werror"
   ```

    Fix all warnings and problems, commit the changes and repeat the previous
    command until no more warnings occur.
5. Tag the release: `git tag -a x.y.z` -- where `x.y.z` represents the
   appropriate version number for the release.
6. Build and test the release tarballs:

   Run: `make distcheck2`

   If that succeeds, check for `fvwm-x.y.z.tar.gz` in the current working
   directory.  This is the release tarball which will be uploaded to Github.
   Unpack it to a temporary directory and build it; check the version as well,
   via: `./fvwm --version`.
7. Push the tag out: `git push origin x.y.z` -- where `x.y.z` is the specific
   tag created in step 5.
8. Set `ISRELEASED` to `"no"` in configure.ac and commit and push that out.
9. Issue a PR (pull-request) against `master` and mege that in assuming all
    checks pass.  If not, fix the problems, and repeat this step.
10. Upload the `fvwm-x.y.z.tar.gz` tarball to Github against the tag just
   pushed.

Updating fvwm-web
=================

1.  Ensure you've a checkout of the repository:

    ```
    git clone git@github.com:fvwmorg/fvwmorg.github.io.git
    ```
2.  Update the `RELEASE` variable in `Makefile` to the desired version which
    has been released.
3.  Run `make`.  This will update all relevant files.
4.  `git commit -a` the result, and push it out.
