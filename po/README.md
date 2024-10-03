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
the fvwm Makefile. All the commands below assume the user is in
the `po/` directory.

## fvwmpo.sh

The shell script `fvwmpo.sh` is the main script to update
the template, initialize new languages, update exist languages,
and build the binary translation files. Each language, `LL_CC`,
has its own translation file.

### Update Template File

To update the main template file, `fvwm3.pot`:

```
./fvwmpo.sh update-pot
```

### Update Existing Translation

To update an existing translation:

1) Update the language file to match the current template
   (if needed):

```
./fvwmpo.sh update LL_CC
```

2) Modify the translation file by updating or adding any
   translation strings in the `LL_CC.po` file.

3) Build the binary translation file:

```
./fvwmpo.sh build LL_CC
```

4) Both the translation `.po` and binary `.gmo` file need to be
   included in the commit to fvwm3.

### Create New Translations

To create a new translation from a template file:

1) Add your language to the `ALL_LINGUAS` variable in
   `../configure.ac` and `fvwmpo.sh` files.

2) Use the template file to create a new LL_CC (e.g., fr, zh_CN)
   translation `.po` file.

```
./fvwmpo.sh init LL_CC
```

3) Follow the above instructions 2-4 to add strings to the translation
   file and build the `.gmo` file to commit to fvwm3.

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

+ FVWM_FILES: This is a list of C source files that use the
  FGettext(string) macro `_(string)`.

+ FVWMRC_FILES: This is a list of files that make use of the
  command expansion `$[gt.string]`. These are used in the default-config
  and various FvwmForms.

+ FVWMSCRIPT_FILES: This is a list of FvwmScript files that use
  `UseGettext` option along with `WindowLocaleTitle` and `LocaleTitle`.

To add new files to be translated, include them in the appropriate
list in `fvwmpo.sh`. Then update the template using the above instructions.
You can also update all translation files and rebuild all `.gmo` files using:

```
./fvwmpo.sh update-all
./fvwmpo.sh build-all
```

