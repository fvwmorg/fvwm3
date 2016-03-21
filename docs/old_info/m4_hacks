Return-Path: dale@felix.dircon.co.uk
Return-Path: <dale@felix.dircon.co.uk>
Received: from eagle.is.lmsc.lockheed.com by rocket (SPCOT.6)
	id AA07582; Wed, 30 Mar 94 21:26:48 EST
Received: from felix.dircon.co.uk by eagle.is.lmsc.lockheed.com (5.65/Ultrix4.3-C)
	id AA27884; Wed, 30 Mar 1994 18:24:30 -0800
Received: from ISOlde.dale.co.uk by tristan.dale.co.uk  with smtp
	(Linux Smail3.1.28.1 #25) id m0pm2IB-0002kOC; Wed, 30 Mar 94 16:34 BST
Received: by ISOlde.dale.co.uk (Linux Smail3.1.28.1 #25)
	id m0pm2IP-000JHaC; Wed, 30 Mar 94 16:35 BST
Message-Id: <m0pm2IP-000JHaC@ISOlde.dale.co.uk>
Date: Wed, 30 Mar 94 16:35 BST
From: Pete.Chown@dale.dircon.co.uk
Subject: fvwm, copyright infringement, m4, etc...

I came up with a hack in m4 to get round the problem where you get
substitution for ordinary words, like 'include'.  I wanted to
substitute a # onto the beginning of every command, so you say
#include as in cpp.  I couldn't do that because only the underscore
and the alphabetics are recognised by m4 as being legitimate in
identifiers.  However, I put an underscore onto the beginning, and
that seems to work quite well.

Given that you refer to the same problem in the documentation for
fvwm, I thought I would send the hack to you:

------ snip ------ snip ------ snip ------ snip ------ snip ------ snip
divert(-1)
changequote(+,-)
changequote(@`,@')
define(_,@`_dnl @')

# We now add an underscore onto the front of all the builtins, to prevent
# unexpected conflicts with words in the text.  The following gross hack does
# this.  Understand it if you can... ;-)

define(def,defn(@`define@'))
define(definition,defn(@`defn@'))
define(remove,defn(@`undefine@'))
define(alias,@`def(@`$2@',definition(@`$1@'))@')
define(hack,@`alias(@`$1@',@`_$1@') remove(@`$1@')@')

hack(@`builtin@')
hack(@`changecom@')
hack(@`changequote@')
hack(@`debugfile@')
hack(@`debugmode@')
hack(@`decr@')
hack(@`define@')
hack(@`defn@')
hack(@`divert@')
hack(@`divnum@')
hack(@`dnl@')
hack(@`dumpdef@')
hack(@`errprint@')
hack(@`esyscmd@')
hack(@`eval@')
hack(@`file@')
hack(@`format@')
hack(@`gnu@')
hack(@`ifdef@')
hack(@`ifelse@')
hack(@`include@')
hack(@`incr@')
hack(@`index@')
hack(@`indir@')
hack(@`len@')
hack(@`line@')
hack(@`m4exit@')
hack(@`m4wrap@')
hack(@`maketemp@')
hack(@`patsubst@')
hack(@`popdef@')
hack(@`pushdef@')
hack(@`regexp@')
hack(@`shift@')
hack(@`sinclude@')
hack(@`substr@')
hack(@`syscmd@')
hack(@`sysval@')
hack(@`traceoff@')
hack(@`traceon@')
hack(@`translit@')
hack(@`undefine@')
hack(@`undivert@')
hack(@`unix@')

_undefine(@`def@')
_undefine(@`definition@')
_undefine(@`alias@')
_undefine(@`hack@')

_divert(0)
------ snip ------ snip ------ snip ------ snip ------ snip ------ snip

(I redefine the quotes as well, because I find that the ordinary
single quote characters are much too common in text that you want to
preprocess.)

One problem with this script is that if someone extends m4 by adding a
new builtin command foo, say, then it will not get an underscore
prepended; this is because you can't get m4 to give you a complete
list of builtins.  For the same reason, the script will probably give
trouble with System V m4, because it won't have definitions for the
GNU extensions.

Anyway, do what you want with the script - if you think it might be
useful to fvwm users you are more than welcome to include it in the
distribution.  Or file it away in /dev/null if you are unimpressed.

I was rather concerned by the little bit of Motif that is getting
distributed along with fvwm.  It may be only a couple of pages out of
10M (is Motif really that big?  Argh!) but that will not stop you
getting sued.  If the Motif people start to get the idea that they are
not selling so many copies because people use fvwm instead of twm,
they will sue you for copyright infringement - not because there is
any particular justice to them protecting two pages of code, but just
because it is the easiest way of making you go away.

It is correct that there is no copyright in structures, only in the
source code that defines them.  So you would be quite within your
rights to rewrite the offending two pages, and then I can't see that
there is anything that the Motif people could do.

-------------------------------------------------------------------------------
Pete.Chown@dale.dircon.co.uk               "The Pen is mightier than the Quill"
                                           -- anonymous
