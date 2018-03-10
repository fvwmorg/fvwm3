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

#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <X11/Intrinsic.h>
#include "FvwmButtons.h"

/**
*** DumpButtons()
*** Debug function. May only be called after ShuffleButtons has been called.
**/
void DumpButtons(button_info *b)
{
	if (!b)
	{
		fprintf(stderr, "NULL\n");
		return;
	}
	if (b != UberButton)
	{
		int button = buttonNum(b);
		fprintf(stderr,
			"0x%lx(%ix%i@(%i,%i)): ",
			(unsigned long)b, b->BWidth, b->BHeight,
			buttonXPos(b, button), buttonYPos(b, button));
	}
	else
	{
		fprintf(stderr,
			"0x%lx(%ix%i@): ",
			(unsigned long)b, b->BWidth, b->BHeight);
	}

	if (b->flags.b_Font)
		fprintf(stderr,
			"Font(%s,0x%lx) ",
			b->font_string, (unsigned long)b->Ffont);
	if (b->flags.b_Padding)
		fprintf(stderr, "Padding(%i,%i) ", b->xpad, b->ypad);
	if (b->flags.b_Frame)
		fprintf(stderr, "Framew(%i) ", b->framew);
	if (b->flags.b_Title)
		fprintf(stderr, "Title(%s) ", b->title);
	if (b->flags.b_Icon)
		fprintf(stderr, "Icon(%s,%i) ", b->icon_file, (int)b->IconWin);
	if (b->flags.b_Icon)
		fprintf(stderr, "Panelw(%i) ", (int)b->PanelWin);
	if (b->flags.b_Action)
		fprintf(stderr,
			"\n  Action(%s,%s,%s,%s) ",
			b->action[0] ? b->action[0] : "",
			b->action[1] ? b->action[1] : "",
			b->action[2] ? b->action[2] : "",
			b->action[3] ? b->action[3] : "");
	if (b->flags.b_Swallow)
	{
		fprintf(stderr, "Swallow(0x%02x) ", b->swallow);
		if (b->swallow & b_Respawn)
			fprintf(stderr, "\n  Respawn(%s) ", b->spawn);
		if (b->newflags.do_swallow_new)
			fprintf(stderr, "\n  SwallowNew(%s) ", b->spawn);
	}
	if (b->flags.b_Panel)
	{
		fprintf(stderr, "Panel(0x%02x) ", b->swallow);
		if (b->swallow & b_Respawn)
			fprintf(stderr, "\n  Respawn(%s) ", b->spawn);
		if (b->newflags.do_swallow_new)
			fprintf(stderr, "\n  SwallowNew(%s) ", b->spawn);
	}
	if (b->flags.b_Hangon)
		fprintf(stderr, "Hangon(%s) ", b->hangon);
	fprintf(stderr, "\n");
	if (b->flags.b_Container)
	{
		int i = 0;
		fprintf(stderr,
			"  Container(%ix%i=%i buttons (alloc %i),"
			" size %ix%i, pos %i,%i)\n{ ",
			b->c->num_columns, b->c->num_rows, b->c->num_buttons,
			b->c->allocated_buttons,
			b->c->width, b->c->height, b->c->xpos, b->c->ypos);
		/*
		  fprintf(stderr,"  font(%s,%i) framew(%i) pad(%i,%i) { ",
		  b->c->font_string,(int)b->c->Ffont,b->c->framew,b->c->xpad,
		  b->c->ypad);
		*/
		while (i<b->c->num_buttons)
		{
			fprintf(stderr,
				"0x%lx ", (unsigned long)b->c->buttons[i++]);
		}
		fprintf(stderr, "}\n");
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
		fprintf(stderr, "%ix%i ", b->BWidth, b->BHeight);
	if (b->flags.b_Font)
		fprintf(stderr, "Font %s ", b->font_string);
	if (b->flags.b_Fore)
		fprintf(stderr, "Fore %s ", b->fore);
	if (b->flags.b_Back)
		fprintf(stderr, "Back %s ", b->back);
	if (b->flags.b_Frame)
		fprintf(stderr, "Frame %i ", b->framew);
	if (b->flags.b_Padding)
		fprintf(stderr, "Padding %i %i ", b->xpad, b->ypad);
	if (b->flags.b_Title)
	{
		fprintf(stderr, "Title ");
		if (b->flags.b_Justify)
		{
			fprintf(stderr, "(");
			switch (b->justify & b_TitleHoriz)
			{
			case 0:
				fprintf(stderr, "Left");
				break;
			case 1:
				fprintf(stderr, "Center");
				break;
			case 2:
				fprintf(stderr, "Right");
				break;
			}
			if (b->justify & b_Horizontal)
			{
				fprintf(stderr, ", Side");
			}
			fprintf(stderr, ") ");
		}
		fprintf(stderr, "\"%s\" ", b->title);
	}
	if (b->flags.b_Icon)
	{
		fprintf(stderr, "Icon \"%s\" ", b->icon_file);
	}
	if (b->flags.b_Swallow || b->flags.b_Panel)
	{
		if (b->flags.b_Swallow)
			fprintf(stderr, "Swallow ");
		else
			fprintf(stderr, "Panel ");
		if (b->swallow_mask)
		{
			fprintf(stderr, "(");
			if (b->swallow_mask & b_NoHints)
			{
				if (b->swallow & b_NoHints)
					fprintf(stderr, "NoHints ");
				else
					fprintf(stderr, "Hints ");
			}

			if (b->swallow_mask & b_Kill)
			{
				if (b->swallow & b_Kill)
					fprintf(stderr, "Kill ");
				else
					fprintf(stderr, "NoKill ");
			}

			if (b->swallow_mask & b_NoClose)
			{
				if (b->swallow & b_NoClose)
					fprintf(stderr, "NoClose ");
				else
					fprintf(stderr, "Close ");
			}

			if (b->swallow_mask & b_Respawn)
			{
				if (b->swallow & b_Respawn)
					fprintf(stderr, "Respawn ");
				else
					fprintf(stderr, "NoRespawn ");
			}

			if (b->swallow_mask & b_UseOld)
			{
				if (b->swallow & b_UseOld)
					fprintf(stderr, "UseOld ");
				else
					fprintf(stderr, "NoOld ");
			}

			if (b->swallow_mask & b_UseTitle)
			{
				if (b->swallow & b_UseTitle)
					fprintf(stderr, "UseTitle ");
				else
					fprintf(stderr, "NoTitle ");
			}

			fprintf(stderr, ") ");
		}
		fprintf(stderr, "\"%s\" \"%s\" ", b->hangon, b->spawn);
	}
	if (b->flags.b_Action)
	{
		if (b->action[0])
			fprintf(stderr, "Action `%s` ", b->action[0]);
		for (i = 1; i < 4; i++)
		{
			if (b->action[i])
				fprintf(stderr,
					"Action (Mouse %i) `%s` ",
					i, b->action[i]);
		}
	}


	if (b->flags.b_Container)
	{
		fprintf(stderr,
			"Container (Columns %i Rows %i ",
			b->c->num_columns, b->c->num_rows);

			if (b->c->flags.b_Font)
				fprintf(stderr, "Font %s ", b->c->font_string);
			if (b->c->flags.b_Fore)
				fprintf(stderr, "Fore %s ", b->c->fore);
			if (b->c->flags.b_Back)
				fprintf(stderr, "Back %s ", b->c->back);
			if (b->c->flags.b_Frame)
				fprintf(stderr, "Frame %i ", b->c->framew);
			if (b->c->flags.b_Padding)
				fprintf(stderr,
					"Padding %i %i ",
					b->c->xpad, b->c->ypad);
			if (b->c->flags.b_Justify)
			{
				fprintf(stderr, "Title (");
				switch (b->c->justify & b_TitleHoriz)
				{
				case 0:
					fprintf(stderr, "Left");
					break;
				case 1:
					fprintf(stderr, "Center");
					break;
				case 2:
					fprintf(stderr, "Right");
					break;
				}
				if (b->c->justify & b_Horizontal)
				{
					fprintf(stderr, ", Side");
				}
				fprintf(stderr, ") ");
			}
			if (b->c->swallow_mask)
			{
				fprintf(stderr, "Swallow (");
				if (b->c->swallow_mask & b_NoHints)
				{
					if (b->c->swallow & b_NoHints)
						fprintf(stderr, "NoHints ");
					else
						fprintf(stderr, "Hints ");
				}

				if (b->c->swallow_mask & b_Kill)
				{
					if (b->c->swallow & b_Kill)
						fprintf(stderr, "Kill ");
					else
						fprintf(stderr, "NoKill ");
				}

				if (b->c->swallow_mask & b_NoClose)
				{
					if (b->c->swallow & b_NoClose)
						fprintf(stderr, "NoClose ");
					else
						fprintf(stderr, "Close ");
				}

				if (b->c->swallow_mask & b_Respawn)
				{
					if (b->c->swallow & b_Respawn)
						fprintf(stderr, "Respawn ");
					else
						fprintf(stderr, "NoRespawn ");
				}

				if (b->c->swallow_mask & b_UseOld)
				{
					if (b->c->swallow & b_UseOld)
						fprintf(stderr, "UseOld ");
					else
						fprintf(stderr, "NoOld ");
				}

				if (b->c->swallow_mask & b_UseTitle)
				{
					if (b->c->swallow & b_UseTitle)
						fprintf(stderr, "UseTitle ");
					else
						fprintf(stderr, "NoTitle ");
				}

				fprintf(stderr, ") ");
			}

		fprintf(stderr, ")");
	}
	fprintf(stderr, "\n");

	if (b->flags.b_Container)
	{
		i = 0;
		while (i<b->c->num_buttons)
		{
			SaveButtons(b->c->buttons[i++]);
		}
		fprintf(stderr, "End\n");
	}
}
