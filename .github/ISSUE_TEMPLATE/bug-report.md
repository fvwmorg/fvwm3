---
name: "Bug Report"
about: "Create a bug to help improve fvwm"
labels: "type:bug"
---

Thanks for reporting your bug here!  The following template will help with
giving as much information as possible so that it's easier to diagnose and
fix.

## Upfront Information

Please provide the following information by running the command and providing
the output.

* Fvwm3 version (run: `fvwm3 --version`)

* Linux distribution or BSD name/version

* Platform (run: `uname -sp`)

## Expected Behaviour

What were you trying to do?  Please explain the problem.

## Actual Behaviour

What should have happened, but didn't?

## Enabling logging

`fvwm3` has a means of logging what it's doing.  Enabling this when
reproducing the issue might help.  To do this, either change the means fvwm3
is started by adding `-v` as in:

```
fvwm3 -v
```

or, once `fvwm3` has loaded, send `SIGUSR2` as in:

```
pkill -USR2 fvwm3
```

The resulting logfile can be found in `$HOME/.fvwm/fvwm3-output.log`

## Steps to Reproduce

How can the problem be reproduced?  For this, the following is helpful:

* Reduce the problem to the smallest `fvwm` configuration example (where
  possible).  Start with a blank config file (`fvwm3 -f/dev/null`) and go from
  there.

* Does the problem also happen with Fvwm2?

Include your configuration with this issue.

## Does Fvwm3 crash?

If fvwm3 crashes then check your system for a corefile.  This is platform
dependant, however, check if:

  - corefiles are enabled (`ulimit -c`)
  - A corefile might be in `$HOME` or `/tmp/`.
  - If you're using Linux (with `systemd`), check `coredumpctl list`.
    `coredumpctl` may need installing separately.

If you find a corefile, install `gdb` and run:

```
gdb /path/to/fvwm3 /path/to/corefile
```

If you're using `coredumpctl` then use:

```
coredumpctl debug
```

Then from within the `(gdb) ` prompt, issue:

```
bt full
```

... and include the output here.

## Extra Information

* Anything else we should know?

* Feel free to take a screen capture or video and upload to this issue if you
  feel it would help.

* Attach `$HOME/.fvwm/fvwm3-output.log` from the step above.
