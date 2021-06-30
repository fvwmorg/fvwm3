# Fvwm Translations

**Help Needed:** Most of the current translations are out of date and mainly
translate unused FvwmForms and FvwmScripts. Help is needed to update the
translations, especially around the default-config and the tools it uses.

Fvwm provides translations for its user interface. This includes a few
internal commands, the default-config, and some module user interfaces.

Fvwm uses `gettext` to produce translations which are direct string
to string translations listed in a `.po` file, which is created
from template file, `.pot`. The `.po` file is then compiled into a
`.gmo` file which is then installed on the system.

This document explains the tools to build template files, update
translations, and to compile the `.gmo` file from the `.po` using
the fvwm Makefile.

## Translator Instructions

Here is a set of instructions to create and update translations.
All of these instructions assume you are in the `po/` directory.

First before you create or update a translation, update the
template files (this ensures you have all the current strings
to translate):

```
make fvwm.pot-update FvwmScript.pot-update
```

### Create New Translations

To create a new translation from a template file:

1. Add your language to the `ALL_LINGUAS` variable in `configure.ac`,
   then `cd po/`.

2. Use the template file to create new LL_CC (e.g., fr, zh_CN)
   `.po` files:

   ```
   msginit -i fvwm.pot -l LL_CC -o fvwm.LL_CC.po
   msginit -i FvwmScript.pot -l LL_CC -o FvwmScript.LL_CC.po
   ```

3. Edit your translation files, `fvwm.LL_CC.po` and `FvwmScript.LL_CC.po`
   by providing the translated string `msgstr` for each string listed.

4. Compile `.gmo` files from the `.po` files:

   ```
   make fvwm.LL_CC.gmo FvwmScript.LL_CC.gmo
   ```

5. Install your translations, then test them with the default-config.

   + If this is the first time installing this translation you have to
     build and install fvwm3 from source, starting with `./autogen.sh`
     to add your language to the Makefiles.
   + Once you have installed the language once, you only need to
     `make install` to install the new `.gmo` file.

   If you notice any issues, repeat steps 3-5 until satisfied.

6. Create a pull request with your updates on github. Be sure to include the
   `.gmo` file in your request.

### Update Existing Translation

To update an existing translation:

1. Use the update template files to update the `.po` files for
   language LL_CC. This will remove old strings and add new strings
   from the template file.

   ```
   make fvwm.LL_CC.po-update FvwmScript.LL_CC.po-update
   ```

2. Follow instructions 3-6 above by updating all the new (and maybe old)
   strings in `fvwm.LL_CC.po` and `FvwmScript.LL_CC.po`.

### Translation Comments

Some menus translation contain a shortcut key character, `&`, as part
of the translation. Place this character where it is most appropriate
in the translation.

FvwmForm and FvwmScript modules are fairly fixed in size, and do not
resize themselves based on text size. This means the translations need
to be close to the same size as the original, otherwise they will get
cutoff, so **be sure to test them**.

FvwmForm XDG help uses spaces to position the text correctly.
Be sure the translated strings have the exact same characters including
spaces. Also if there are leading spaces, ensure those are included in
the translation.

There is a translation that is just a set number of spaces. This is used
for indention in the FvwmForm XDG help. You can increase or decrease the
number of spaces in your translation to help position the translation.

## Developer Comments

Adding new strings to be translated to the templates depends on where
the string is located. Right now there are three possible file types
the Makefiles use to build the templates:

+ FVWM_POT_FILES: This is a list of C source files that use the
  FGettext(string) macro `_(string)`.

+ FVWMRC_POT_FILES: This is a list of files that make use of the
  command expansion `$[gt.string]`. These are used in the default-config
  and various FvwmForms.

+ FVWMSCRIPT_POT_FILES: This is a list of FvwmScript files that use
  `UseGettext` option along with `WindowLocaleTitle` and `LocaleTitle`.

To add new files to be translated, include them in the appropriate
list in `po/MakeFile.am`. Then run `autogen.sh` and `./configure`
to regenerate the Makefiles. Afterwards you can update the template
with the new strings using the commands above.

