FVWM3
-----

[![Codacy Status](https://api.codacy.com/project/badge/Grade/0bfafcfd16b84a96a1305e92de2d6c4c)](https://app.codacy.com/gh/fvwmorg/fvwm3?utm_source=github.com&utm_medium=referral&utm_content=fvwmorg/fvwm3&utm_campaign=Badge_Grade_Settings)
![Build Status](https://github.com/fvwmorg/fvwm3/workflows/FVWM3%20CI/badge.svg)
![Open issues](https://img.shields.io/github/issues-raw/fvwmorg/fvwm3)
![GitHub contributors](https://img.shields.io/github/contributors/fvwmorg/fvwm3)

Welcome to fvwm.  Version 3 is a multiple large virtual desktop window manager.

The successor to [fvwm-2.6.x](http://github.com/fvwmorg/fvwm).

Fvwm3 is intended to be extremely customizable and extendible while consuming
a relatively small amount of resources.

Fvwm2 Compatibility
-------------------

Although an existing fvwm2 configuration should mostly work with fvwm3, it
isn't guaranteed to work without some modifications.  This is because fvwm3
has changed, often removing long-standing deprecated options.

More details about those changes [can be found here](https://github.com/fvwmorg/fvwm3/discussions/878)

Releases / Changelog
--------------------

* [See the CHANGELOG](./CHANGELOG.md)

Installation
------------

See [the installation instructions](./INSTALL.md)

Help & Support
--------------

We have a strong community on IRC (libera.chat), in the `#fvwm` channel if you
fancy a chat.

There is also the [Fvwm Forums](https://fvwmforums.org) where you can ask
questions.

Issues (for bugs) can be opened, and any/all bug reports are appreciated!

Development
-----------

Those interested in contributing to FVWM3 should have [a read of the developer
documentation](./dev-docs/DEVELOPERS.md).

* A [TODO file](./dev-docs/TODO.md) exists, and sometimes even things from it are
worked on.

* New commands and config file syntax thoughts [are being documented here](./dev-docs/NEW-COMMANDS.md).

Please open a github issue or contact me directly if you wish to discuss a
particular feature of issue you need help with.

AI Policy
---------

FVWM is an old project -- well over 30 years of development.  It came at a
time when the people working on it *really* knew their stuff.  They had to
think about how fvwm interacted with xlib/xserver implementations, and work a
lot things out for themselves.

All of it was done without AI, obviously.

It's absolutely important that FVWM continues to be maintained with this
history in mind, to introduce AI would destroy that approach in a way which
also destroys the people who have worked on it over the many years it has been
alive.

It's important we protect this at all costs.

Therefore, the use of AI in this project means that it cannot be used for:

* Code generation
* Opening issues
* Writing/creating PRs (Pull Requests)
* Creating documentation updates

Such examples where this is submitted will be ignored/closed outright.
