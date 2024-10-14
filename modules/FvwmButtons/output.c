/* -*-c-*- */
/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 */

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>
 */

#include "config.h"

#include "libs/log.h"
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include "FvwmButtons.h"

/**
*** DumpButtons()
*** Debug function. May only be called after ShuffleButtons has been called.
**/
void DumpButtons(button_info *b)
{
	if (!b)
	{
		fvwm_debug(__func__, "NULL\n");
		return;
	}
	if (b != UberButton)
	{
		int button = buttonNum(b);
		fvwm_debug(__func__,
			   "0x%lx(%ix%i@(%i,%i)): ",
			   (unsigned long)b, b->BWidth, b->BHeight,
			   buttonXPos(b, button), buttonYPos(b, button));
	}
	else
	{
		fvwm_debug(__func__,
			   "0x%lx(%ix%i@): ",
			   (unsigned long)b, b->BWidth, b->BHeight);
	}

	if (b->flags.b_Font)
		fvwm_debug(__func__,
			   "Font(%s,0x%lx) ",
			   b->font_string, (unsigned long)b->Ffont);
	if (b->flags.b_Padding)
		fvwm_debug(__func__, "Padding(%i,%i) ", b->xpad, b->ypad);
	if (b->flags.b_Frame)
		fvwm_debug(__func__, "Framew(%i) ", b->framew);
	if (b->flags.b_Title)
		fvwm_debug(__func__, "Title(%s) ", b->title);
	if (b->flags.b_Icon)
		fvwm_debug(__func__, "Icon(%s,%i) ", b->icon_file,
			   (int)b->IconWin);
	if (b->flags.b_Icon)
		fvwm_debug(__func__, "Panelw(%i) ", (int)b->PanelWin);
	if (b->flags.b_Action)
		fvwm_debug(__func__,
			   "\n  Action(%s,%s,%s,%s) ",
			   b->action[0] ? b->action[0] : "",
			   b->action[1] ? b->action[1] : "",
			   b->action[2] ? b->action[2] : "",
			   b->action[3] ? b->action[3] : "");
	if (b->flags.b_Swallow)
	{
		fvwm_debug(__func__, "Swallow(0x%02x) ", b->swallow);
		if (b->swallow & b_Respawn)
			fvwm_debug(__func__, "\n  Respawn(%s) ", b->spawn);
		if (b->newflags.do_swallow_new)
			fvwm_debug(__func__, "\n  SwallowNew(%s) ", b->spawn);
	}
	if (b->flags.b_Panel)
	{
		fvwm_debug(__func__, "Panel(0x%02x) ", b->swallow);
		if (b->swallow & b_Respawn)
			fvwm_debug(__func__, "\n  Respawn(%s) ", b->spawn);
		if (b->newflags.do_swallow_new)
			fvwm_debug(__func__, "\n  SwallowNew(%s) ", b->spawn);
	}
	if (b->flags.b_Hangon)
		fvwm_debug(__func__, "Hangon(%s) ", b->hangon);
	fvwm_debug(__func__, "\n");
	if (b->flags.b_Container)
	{
		int i = 0;
		fvwm_debug(__func__,
			   "  Container(%ix%i=%i buttons (alloc %i),"
			   " size %ix%i, pos %i,%i)\n{ ",
			   b->c->num_columns, b->c->num_rows,
			   b->c->num_buttons,
			   b->c->allocated_buttons,
			   b->c->width, b->c->height, b->c->xpos, b->c->ypos);
		/*
		  fprintf(stderr,"  font(%s,%i) framew(%i) pad(%i,%i) { ",
		  b->c->font_string,(int)b->c->Ffont,b->c->framew,b->c->xpad,
		  b->c->ypad);
		*/
		while (i<b->c->num_buttons)
		{
			fvwm_debug(__func__,
				   "0x%lx ",
				   (unsigned long)b->c->buttons[i++]);
		}
		fvwm_debug(__func__, "}\n");
		i = 0;
		while (i < b->c->num_buttons)
		{
			DumpButtons(b->c->buttons[i++]);
		}
		return;
	}
}

void SaveButtons(button_info *b)
{
	int i;
	if (!b)
		return;
	if (b->BWidth>1 || b->BHeight > 1)
		fvwm_debug(__func__, "%ix%i ", b->BWidth, b->BHeight);
	if (b->flags.b_Font)
		fvwm_debug(__func__, "Font %s ", b->font_string);
	if (b->flags.b_Fore)
		fvwm_debug(__func__, "Fore %s ", b->fore);
	if (b->flags.b_Back)
		fvwm_debug(__func__, "Back %s ", b->back);
	if (b->flags.b_Frame)
		fvwm_debug(__func__, "Frame %i ", b->framew);
	if (b->flags.b_Padding)
		fvwm_debug(__func__, "Padding %i %i ", b->xpad, b->ypad);
	if (b->flags.b_Title)
	{
		fvwm_debug(__func__, "Title ");
		if (b->flags.b_Justify)
		{
			fvwm_debug(__func__, "(");
			switch (b->justify & b_TitleHoriz)
			{
			case 0:
				fvwm_debug(__func__, "Left");
				break;
			case 1:
				fvwm_debug(__func__, "Center");
				break;
			case 2:
				fvwm_debug(__func__, "Right");
				break;
			}
			if (b->justify & b_Horizontal)
			{
				fvwm_debug(__func__, ", Side");
			}
			fvwm_debug(__func__, ") ");
		}
		fvwm_debug(__func__, "\"%s\" ", b->title);
	}
	if (b->flags.b_Icon)
	{
		fvwm_debug(__func__, "Icon \"%s\" ", b->icon_file);
	}
	if (b->flags.b_Swallow || b->flags.b_Panel)
	{
		if (b->flags.b_Swallow)
			fvwm_debug(__func__, "Swallow ");
		else
			fvwm_debug(__func__, "Panel ");
		if (b->swallow_mask)
		{
			fvwm_debug(__func__, "(");
			if (b->swallow_mask & b_NoHints)
			{
				if (b->swallow & b_NoHints)
					fvwm_debug(__func__, "NoHints ");
				else
					fvwm_debug(__func__, "Hints ");
			}

			if (b->swallow_mask & b_Kill)
			{
				if (b->swallow & b_Kill)
					fvwm_debug(__func__, "Kill ");
				else
					fvwm_debug(__func__, "NoKill ");
			}

			if (b->swallow_mask & b_NoClose)
			{
				if (b->swallow & b_NoClose)
					fvwm_debug(__func__, "NoClose ");
				else
					fvwm_debug(__func__, "Close ");
			}

			if (b->swallow_mask & b_Respawn)
			{
				if (b->swallow & b_Respawn)
					fvwm_debug(__func__, "Respawn ");
				else
					fvwm_debug(__func__, "NoRespawn ");
			}

			if (b->swallow_mask & b_UseOld)
			{
				if (b->swallow & b_UseOld)
					fvwm_debug(__func__, "UseOld ");
				else
					fvwm_debug(__func__, "NoOld ");
			}

			if (b->swallow_mask & b_UseTitle)
			{
				if (b->swallow & b_UseTitle)
					fvwm_debug(__func__, "UseTitle ");
				else
					fvwm_debug(__func__, "NoTitle ");
			}

			fvwm_debug(__func__, ") ");
		}
		fvwm_debug(__func__, "\"%s\" \"%s\" ", b->hangon, b->spawn);
	}
	if (b->flags.b_Action)
	{
		if (b->action[0])
			fvwm_debug(__func__, "Action `%s` ", b->action[0]);
		for (i = 1; i < 4; i++)
		{
			if (b->action[i])
				fvwm_debug(__func__,
					   "Action (Mouse %i) `%s` ",
					   i, b->action[i]);
		}
	}


	if (b->flags.b_Container)
	{
		fvwm_debug(__func__,
			   "Container (Columns %i Rows %i ",
			   b->c->num_columns, b->c->num_rows);

			if (b->c->flags.b_Font)
				fvwm_debug(__func__, "Font %s ",
					   b->c->font_string);
			if (b->c->flags.b_Fore)
				fvwm_debug(__func__, "Fore %s ", b->c->fore);
			if (b->c->flags.b_Back)
				fvwm_debug(__func__, "Back %s ", b->c->back);
			if (b->c->flags.b_Frame)
				fvwm_debug(__func__, "Frame %i ",
					   b->c->framew);
			if (b->c->flags.b_Padding)
				fvwm_debug(__func__,
					   "Padding %i %i ",
					   b->c->xpad, b->c->ypad);
			if (b->c->flags.b_Justify)
			{
				fvwm_debug(__func__, "Title (");
				switch (b->c->justify & b_TitleHoriz)
				{
				case 0:
					fvwm_debug(__func__, "Left");
					break;
				case 1:
					fvwm_debug(__func__, "Center");
					break;
				case 2:
					fvwm_debug(__func__, "Right");
					break;
				}
				if (b->c->justify & b_Horizontal)
				{
					fvwm_debug(__func__, ", Side");
				}
				fvwm_debug(__func__, ") ");
			}
			if (b->c->swallow_mask)
			{
				fvwm_debug(__func__, "Swallow (");
				if (b->c->swallow_mask & b_NoHints)
				{
					if (b->c->swallow & b_NoHints)
						fvwm_debug(__func__,
							   "NoHints ");
					else
						fvwm_debug(__func__, "Hints ");
				}

				if (b->c->swallow_mask & b_Kill)
				{
					if (b->c->swallow & b_Kill)
						fvwm_debug(__func__, "Kill ");
					else
						fvwm_debug(__func__,
							   "NoKill ");
				}

				if (b->c->swallow_mask & b_NoClose)
				{
					if (b->c->swallow & b_NoClose)
						fvwm_debug(__func__,
							   "NoClose ");
					else
						fvwm_debug(__func__, "Close ");
				}

				if (b->c->swallow_mask & b_Respawn)
				{
					if (b->c->swallow & b_Respawn)
						fvwm_debug(__func__,
							   "Respawn ");
					else
						fvwm_debug(__func__,
							   "NoRespawn ");
				}

				if (b->c->swallow_mask & b_UseOld)
				{
					if (b->c->swallow & b_UseOld)
						fvwm_debug(__func__,
							   "UseOld ");
					else
						fvwm_debug(__func__, "NoOld ");
				}

				if (b->c->swallow_mask & b_UseTitle)
				{
					if (b->c->swallow & b_UseTitle)
						fvwm_debug(__func__,
							   "UseTitle ");
					else
						fvwm_debug(__func__,
							   "NoTitle ");
				}

				fvwm_debug(__func__, ") ");
			}

		fvwm_debug(__func__, ")");
	}
	fvwm_debug(__func__, "\n");

	if (b->flags.b_Container)
	{
		i = 0;
		while (i<b->c->num_buttons)
		{
			SaveButtons(b->c->buttons[i++]);
		}
		fvwm_debug(__func__, "End\n");
	}
}
