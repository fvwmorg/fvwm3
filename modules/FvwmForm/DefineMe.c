#include "config.h"
#include "libs/fvwmlib.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include <FvwmForm.h>

/*
 * Send commands to fvwm to define this module.
 *
 * This creates the builtin Form "FormFvwmForm".
 */
/* Macros for creating and sending commands:
   CMD0X - Just send
   CMD0V - One var
   */
#define CMD0X(TEXT) \
  SendText(Channel,TEXT,0);
#define CMD0V(TEXT,VAR) \
  sprintf(cmd,TEXT,VAR);\
  SendText(Channel,cmd,0);

void DefineMe() {
  char cmd[500];                        /* really big area for a command */
  myfprintf((stderr,"defining FormFvwmForm\n"));
  CMD0X("DestroyModuleConfig FormFvwmForm*");
  CMD0X("*FormFvwmFormWarpPointer");
  CMD0X("*FormFvwmFormLine  center");
  CMD0X("*FormFvwmFormText  \"FvwmForm Set Form Defaults:\"");
  CMD0X("*FormFvwmFormLine  left");
  CMD0X("*FormFvwmFormText  \"\"");
  CMD0X("*FormFvwmFormLine  left");
  CMD0X("*FormFvwmFormText  \"Font for text:  \"");
  CMD0V("*FormFvwmFormInput TextFont 60 \"%s\"",font_names[f_text]);
  CMD0X("*FormFvwmFormLine  left");
  CMD0X("*FormFvwmFormText  \"Font for input: \"");
  CMD0V("*FormFvwmFormInput InputFont 60 \"%s\"",font_names[f_input]);
  CMD0X("*FormFvwmFormLine  left");
  CMD0X("*FormFvwmFormText  \"Font for button:\"");
  CMD0V("*FormFvwmFormInput ButtonFont 60 \"%s\"",font_names[f_button]);
                            
  CMD0X("*FormFvwmFormLine  left");
  CMD0X("*FormFvwmFormText  \"Color of text f/g:   \"");
  CMD0V("*FormFvwmFormInput TextColorFG 20 \"%s\"",color_names[c_fg]);
  CMD0X("*FormFvwmFormText  \"b/g:\"");
  CMD0V("*FormFvwmFormInput TextColorBG 20 \"%s\"",color_names[c_bg]);
  CMD0X("*FormFvwmFormLine  left");
  CMD0X("*FormFvwmFormText  \"Color of buttons f/g:\"");
  CMD0V("*FormFvwmFormInput ButtonFG 20 \"%s\"",color_names[c_item_fg]);
  CMD0X("*FormFvwmFormText  \"b/g:\"");
  CMD0V("*FormFvwmFormInput ButtonBG 20 \"%s\"",color_names[c_item_bg]);

  CMD0X("*FormFvwmFormLine left");
  /* Testing, there is no big need for a message in this form,
     none of the fields generate fvwm messages anyway. */
  CMD0X("*FormFvwmFormMessage Len 50, Font 10x10, Fore Red");

  /*
    F1 - Save and Restart This Form. Generates commands:
    1. Clear fvwm's config.
    2. Synchronously write the new config to a file.
    3. Tell next copy of me to reread config file.
    4. Tell fvwm to start a new copy of this module.
  */

  CMD0X("*FormFvwmFormLine         expand");
  CMD0X("*FormFvwmFormButton       quit \"F1 - Save & Restart This Form\" F1");
  CMD0X("*FormFvwmFormCommand DestroyModuleConfig FormFvwmFormDefault*");
  CMD0X("*FormFvwmFormCommand !/bin/echo \"# This file last created by\
 FvwmForm on: `/bin/date`.\n\
*FvwmFormDefaultFont $(TextFont)\n\
*FvwmFormDefaultInputFont $(InputFont)\n\
*FvwmFormDefaultButtonFont $(ButtonFont)\n\
*FvwmFormDefaultFore $(TextColorFG)\n\
*FvwmFormDefaultBack $(TextColorBG)\n\
*FvwmFormDefaultItemFore $(ButtonFG)\n\
*FvwmFormDefaultItemBack $(ButtonBG)\
\" > .FvwmForm");
  CMD0X("*FormFvwmFormCommand *FvwmFormDefaultRead n"); /* force reread */
  CMD0X("*FormFvwmFormCommand Module FvwmForm FormFvwmForm"); /* copy of me! */

  /* F3 - Reset */
  CMD0X("*FormFvwmFormButton       restart   \"F3 - Reset\" F3");

  /* F4 - Dismiss */
  CMD0X("*FormFvwmFormButton       quit \"F4 - Dismiss\" F4");
  CMD0X("*FormFvwmFormCommand Nop");
}
