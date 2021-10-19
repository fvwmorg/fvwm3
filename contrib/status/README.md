Scripts
=======

In this directory are a few files:

`.`:

* panel -- a wrapper sh script which chains together a perl script and
  lemonbar, so that a panel can be formed.  Has a crude understanding of job
  control so that only one instance can be started at any one time.  It will
  also start conky.

* read_status.pl -- listens on any defined FIFOs from `fvwm3`, and parses the
  JSON information sent down them.  It formats the information into
  something which `lemonbar` can understand.  It will also send the conky output
  to `lemonbar`.

* conkyrc -- example RC file for use with conky.

`./config`
* Example config(s)

In order to support RandR correctly, a forked version of Lemonbar is needed.
The version lives on the `ta/keep-output-monitor` see:

https://github.com/ThomasAdam/bar/tree/ta/keep-output-monitor

Compiling this should replace any other version of `lemonbar` which might be in
use.
