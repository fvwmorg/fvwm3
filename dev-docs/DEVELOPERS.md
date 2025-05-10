<!--- IF THIS DOCUMENT IS UPDATED THEN ALSO UPDATE THE FVWM WEB REPO --->

Developing for FVWM
===================

This document aims to help the developer with the expectations when dealing
with the FVWM3 source code.

The FVWM3 source conforms to the [Linux kernel style guide](
https://www.kernel.org/doc/html/latest/process/coding-style.html).

Command Parsing
===============

The internal representation of how fvwm3 parses commands in undergoing a
rewrite.  [Some notes on how fvwm3 parses commands exists](PARSING.md).

Branch Workflows / Submitting Code Changes
==========================================

The main FVWM3 repository treats the `main` branch as stable, in that it's the
branch which has the most tested code on it, and the branch from which releases
are made.  Formal releases of FVWM3 are tagged, in the form `x.y.z`, historical
versions of FVWM3 are tagged as `version-x_y_z`.  Untagged code may well
accumulate on `main`, which will go to form the next release.

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
of writing a new feature, it is worthwhile opening an issue against the `fvwm3`
repository to discuss whether it's a good idea, and to check no one else is
working on that feature.

Those wishing to submit code/bug-fixes should:

* [Fork the FVWM3-repository](https://github.com/fvwmorg/fvwm3#fork-destination-box)
* Add the [FVWM3-repo](https://github.com/fvwmorg/fvwm3.git) as an upstream
  remote:
  * `git remote add fvwmorg https://github.com/fvwmorg/fvwm3.git &&
    git fetch fvwmorg`
* Create a topic-branch to house your work:
  * `git switch -b initial/mybranch`
  * [ hack, hack, hack... ] && commit
* Rebase it against `fvwmorg/main`:
  * `git fetch && git rebase -i fvwmorg/main`
* Push the latest changes to your fork:
  * If you've never pushed this branch before:  `git push -u origin HEAD`
  * Or, if updating an existing branch: `git push origin -f`
* Open a pull-request

### Protected branches and the use of Github Actions

Pull-requests made will result in the use of Github Actions being run against the
branch.  This builds the copy of the pushed code in a Debian environment, with
all the additional libraries FVWM could use, loaded in.  If a build fails this
check, it is recommend to fix this by rebasing the commits with the additional
fixes

The FVWM3 repository also treats the `main` branch as protected.  This is a
[GitHub feature](https://help.github.com/articles/about-protected-branches/)
which means the `main` branch in this case cannot have changes merged into it
until Github Actions has verified the builds do not fail.

This has merit since not every developer will be using the same operating
systems (Linux versus BSD for instance), and that `main` is meant to try and
be as release-worthy as can be.

**NOTE**:  This means that no work can be committed to `main` directly.  ALL
work that needs to appear on `main`---including the release
process---**MUST** go via a separate topic-branch, with a PR (pull-request).
Not even fvwmorg owners are an exception to this.

### Merging changes / Pull Requests

The history of `main` should be as linear as possible, therefore when
merging changes to it the branch(es) in question should be rebased against
main first of all.  This will stop a merge commit from happening.

If using github this process is easy, since the `Merge pull request` button
has an option to `Rebase and Merge`.  This is what should be used.  See also
[the documentation on Github](https://github.com/blog/2243-rebase-and-merge-pull-requests)

If this is manual (which will only work when the Github Actions checks have
passed), then:

```
git checkout topic/branch
git fetch --all
git rebase -i origin/main
git checkout main
git merge topic/branch
git push
```
Conventions
===========

The following tries to list all the conventions that the fvwm developers
adhere to, either by consensus through discussion, common practice or unspoken
agreement.  It is hopefully useful for the fvwm development newbie.

Programming Languages
--------------------

 The following programming languages are allowed:

- ANSI C
- Perl
- Portable /bin/sh scripts for examples.

Editorconfig
------------

At the top-level of the `fvwm3` git repo, is a [.editorconfig](../.editorconfig)
file which sets some options which can be used across different editors.  See
the [editorconfig webpage](https://editorconfig.org/) for more information and
to see whether your editor is supported.

New Code Files
--------------

- All .c files *must* have

```
#include "config.h"
```

as the first non-comment line.

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

- Every feature must be described with all options in the man page.  Man pages
  are generated via Asciidoctor from files in `doc/`

Creating a release
==================

Releases are created entirely automatically from Github Actions.

The `.github/workflows/release.yml` contains the code which underpins this.

To create an actual release, means going to the following:

https://github.com/fvwmorg/fvwm3/actions/workflows/release.yml

The branch defaults to `main` which is where we want to generate the release
from. Don't change this.

The input will ask for the next version fvwm3 will be after release.  For
instance, if the current version is `1.1.3`, then the correct input to use is
`1.1.4`.

If you want to find the current release fvwm3 is at, see the
`.current-release` file.

**Unfortunately there is no way of programatically showing the current version,
so this needs to be looked up manually before confirming the new versioon
befpre running this workflow.**

There's a second input, defaulting to "false", asking whether the release
should be published as a draft or not.  Generally, this shouldn't need to be
set to `true` -- but it's left in as a means of allowing the person doing the
release some degree of flexibility.

Note that the act of publishing a release -- whether it's a draft or not, will
still tag the release and bump `fvwm3` to the next version,.  This is also by
design.

If a release has to be re-rolled, then it is up to the person doing that
release to delete it and recreate it, perhaps via the following commands:

```
git tag -d <TAG_OF_RELEASE> ; git push origin --delete <TAG_OF_RELEASE>
```

The specific release of `<TAG_OF_RELEASE>` would also have to be removed.
This can be done directly on GitHub, or via the `gh` client:

```
gh release delete <TAG_OF_RELEASE>
```

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
