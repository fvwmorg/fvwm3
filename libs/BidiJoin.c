/* -*-c-*- */
/* Copyright (C) 2002  Nadim Shaikli (arabeyes.org) */
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

/*
 * FBidiJoin.c
 *
 * A place holder for when fribidi finally implements character
 * shaping/joining.  This shaping/joining is a must for a variety of
 * Bidi'ed languages such as Arabic, Farsi, as well as all the Syriacs
 * (Urdu, Sindhi, Pashto, Kurdish, Baluchi, Kashmiri, Kazakh, Berber,
 * Uighur, Kirghiz, etc).
 */

#include "config.h"

#if HAVE_BIDI

#include "fvwmlib.h"
#include "BidiJoin.h"

/* ---------------------------- included header files ---------------------- */

#include <stdio.h>

/* ---------------------------- local types -------------------------------- */

typedef struct char_shaped
{
	FriBidiChar base;

	/* The various Arabic shaped permutations */
	FriBidiChar isolated;
	FriBidiChar initial;
	FriBidiChar medial;
	FriBidiChar final;
} char_shaped_t;

typedef struct char_shaped_comb
{
	/* The Arabic exceptions - 2chars ==> 1char */
	FriBidiChar first;
	FriBidiChar second;
	FriBidiChar comb_isolated;
	FriBidiChar comb_joined;
} char_shaped_comb_t;

/* ---------------------------- static variables --------------------------- */

static const char_shaped_t shaped_table[] =
{
	/*  base       s       i       m       f */
	{ 0x0621, 0xFE80, 0x0000, 0x0000, 0x0000, },  /* HAMZA */
	{ 0x0622, 0xFE81, 0x0000, 0x0000, 0xFE82, },  /* ALEF_MADDA */
	{ 0x0623, 0xFE83, 0x0000, 0x0000, 0xFE84, },  /* ALEF_HAMZA_ABOVE */
	{ 0x0624, 0xFE85, 0x0000, 0x0000, 0xFE86, },  /* WAW_HAMZA */
	{ 0x0625, 0xFE87, 0x0000, 0x0000, 0xFE88, },  /* ALEF_HAMZA_BELOW */
	{ 0x0626, 0xFE89, 0xFE8B, 0xFE8C, 0xFE8A, },  /* YEH_HAMZA */
	{ 0x0627, 0xFE8D, 0x0000, 0x0000, 0xFE8E, },  /* ALEF */
	{ 0x0628, 0xFE8F, 0xFE91, 0xFE92, 0xFE90, },  /* BEH */
	{ 0x0629, 0xFE93, 0x0000, 0x0000, 0xFE94, },  /* TEH_MARBUTA */
	{ 0x062A, 0xFE95, 0xFE97, 0xFE98, 0xFE96, },  /* TEH */
	{ 0x062B, 0xFE99, 0xFE9B, 0xFE9C, 0xFE9A, },  /* THEH */
	{ 0x062C, 0xFE9D, 0xFE9F, 0xFEA0, 0xFE9E, },  /* JEEM */
	{ 0x062D, 0xFEA1, 0xFEA3, 0xFEA4, 0xFEA2, },  /* HAH */
	{ 0x062E, 0xFEA5, 0xFEA7, 0xFEA8, 0xFEA6, },  /* KHAH */
	{ 0x062F, 0xFEA9, 0x0000, 0x0000, 0xFEAA, },  /* DAL */
	{ 0x0630, 0xFEAB, 0x0000, 0x0000, 0xFEAC, },  /* THAL */
	{ 0x0631, 0xFEAD, 0x0000, 0x0000, 0xFEAE, },  /* REH */
	{ 0x0632, 0xFEAF, 0x0000, 0x0000, 0xFEB0, },  /* ZAIN */
	{ 0x0633, 0xFEB1, 0xFEB3, 0xFEB4, 0xFEB2, },  /* SEEN */
	{ 0x0634, 0xFEB5, 0xFEB7, 0xFEB8, 0xFEB6, },  /* SHEEN */
	{ 0x0635, 0xFEB9, 0xFEBB, 0xFEBC, 0xFEBA, },  /* SAD */
	{ 0x0636, 0xFEBD, 0xFEBF, 0xFEC0, 0xFEBE, },  /* DAD */
	{ 0x0637, 0xFEC1, 0xFEC3, 0xFEC4, 0xFEC2, },  /* TAH */
	{ 0x0638, 0xFEC5, 0xFEC7, 0xFEC8, 0xFEC6, },  /* ZAH */
	{ 0x0639, 0xFEC9, 0xFECB, 0xFECC, 0xFECA, },  /* AIN */
	{ 0x063A, 0xFECD, 0xFECF, 0xFED0, 0xFECE, },  /* GHAIN */
	{ 0x0640, 0x0640, 0x0640, 0x0640, 0x0640, },  /* TATWEEL */
	{ 0x0641, 0xFED1, 0xFED3, 0xFED4, 0xFED2, },  /* FEH */
	{ 0x0642, 0xFED5, 0xFED7, 0xFED8, 0xFED6, },  /* QAF */
	{ 0x0643, 0xFED9, 0xFEDB, 0xFEDC, 0xFEDA, },  /* KAF */
	{ 0x0644, 0xFEDD, 0xFEDF, 0xFEE0, 0xFEDE, },  /* LAM */
	{ 0x0645, 0xFEE1, 0xFEE3, 0xFEE4, 0xFEE2, },  /* MEEM */
	{ 0x0646, 0xFEE5, 0xFEE7, 0xFEE8, 0xFEE6, },  /* NOON */
	{ 0x0647, 0xFEE9, 0xFEEB, 0xFEEC, 0xFEEA, },  /* HEH */
	{ 0x0648, 0xFEED, 0x0000, 0x0000, 0xFEEE, },  /* WAW */
	{ 0x0649, 0xFEEF, 0xFBE8, 0xFBE9, 0xFEF0, },  /* ALEF_MAKSURA */
	{ 0x064A, 0xFEF1, 0xFEF3, 0xFEF4, 0xFEF2, },  /* YEH */
	{ 0x0671, 0xFB50, 0x0000, 0x0000, 0xFB51, },
	{ 0x0679, 0xFB66, 0xFB68, 0xFB69, 0xFB67, },
	{ 0x067A, 0xFB5E, 0xFB60, 0xFB61, 0xFB5F, },
	{ 0x067B, 0xFB52, 0xFB54, 0xFB55, 0xFB53, },
	{ 0x067E, 0xFB56, 0xFB58, 0xFB59, 0xFB57, },
	{ 0x067F, 0xFB62, 0xFB64, 0xFB65, 0xFB63, },
	{ 0x0680, 0xFB5A, 0xFB5C, 0xFB5D, 0xFB5B, },
	{ 0x0683, 0xFB76, 0xFB78, 0xFB79, 0xFB77, },
	{ 0x0684, 0xFB72, 0xFB74, 0xFB75, 0xFB73, },
	{ 0x0686, 0xFB7A, 0xFB7C, 0xFB7D, 0xFB7B, },
	{ 0x0687, 0xFB7E, 0xFB80, 0xFB81, 0xFB7F, },
	{ 0x0688, 0xFB88, 0x0000, 0x0000, 0xFB89, },
	{ 0x068C, 0xFB84, 0x0000, 0x0000, 0xFB85, },
	{ 0x068D, 0xFB82, 0x0000, 0x0000, 0xFB83, },
	{ 0x068E, 0xFB86, 0x0000, 0x0000, 0xFB87, },
	{ 0x0691, 0xFB8C, 0x0000, 0x0000, 0xFB8D, },
	{ 0x0698, 0xFB8A, 0x0000, 0x0000, 0xFB8B, },
	{ 0x06A4, 0xFB6A, 0xFB6C, 0xFB6D, 0xFB6B, },
	{ 0x06A6, 0xFB6E, 0xFB70, 0xFB71, 0xFB6F, },
	{ 0x06A9, 0xFB8E, 0xFB90, 0xFB91, 0xFB8F, },
	{ 0x06AD, 0xFBD3, 0xFBD5, 0xFBD6, 0xFBD4, },
	{ 0x06AF, 0xFB92, 0xFB94, 0xFB95, 0xFB93, },
	{ 0x06B1, 0xFB9A, 0xFB9C, 0xFB9D, 0xFB9B, },
	{ 0x06B3, 0xFB96, 0xFB98, 0xFB99, 0xFB97, },
	{ 0x06BB, 0xFBA0, 0xFBA2, 0xFBA3, 0xFBA1, },
	{ 0x06BE, 0xFBAA, 0xFBAC, 0xFBAD, 0xFBAB, },
	{ 0x06C0, 0xFBA4, 0x0000, 0x0000, 0xFBA5, },
	{ 0x06C1, 0xFBA6, 0xFBA8, 0xFBA9, 0xFBA7, },
	{ 0x06C5, 0xFBE0, 0x0000, 0x0000, 0xFBE1, },
	{ 0x06C6, 0xFBD9, 0x0000, 0x0000, 0xFBDA, },
	{ 0x06C7, 0xFBD7, 0x0000, 0x0000, 0xFBD8, },
	{ 0x06C8, 0xFBDB, 0x0000, 0x0000, 0xFBDC, },
	{ 0x06C9, 0xFBE2, 0x0000, 0x0000, 0xFBE3, },
	{ 0x06CB, 0xFBDE, 0x0000, 0x0000, 0xFBDF, },
	{ 0x06CC, 0xFBFC, 0xFBFE, 0xFBFF, 0xFBFD, },
	{ 0x06D0, 0xFBE4, 0xFBE6, 0xFBE7, 0xFBE5, },
	{ 0x06D2, 0xFBAE, 0x0000, 0x0000, 0xFBAF, },
	{ 0x06D3, 0xFBB0, 0x0000, 0x0000, 0xFBB1, },
	/* special treatment for ligatures from combining phase */
	{ 0xFEF5, 0xFEF5, 0x0000, 0x0000, 0xFEF6, }, /* LAM_ALEF_MADDA */
	{ 0xFEF7, 0xFEF7, 0x0000, 0x0000, 0xFEF8, }, /* LAM_ALEF_HAMZA_ABOVE */
	{ 0xFEF9, 0xFEF9, 0x0000, 0x0000, 0xFEFA, }, /* LAM_ALEF_HAMZA_BELOW */
	{ 0xFEFB, 0xFEFB, 0x0000, 0x0000, 0xFEFC, }, /* LAM_ALEF */
};

/* -------------------------- local functions ------------------------------ */

static const char_shaped_t *
get_shaped_entry(FriBidiChar ch)
{
	int count;
	int table_size;

	table_size = sizeof(shaped_table) / sizeof(shaped_table[0]);

	for (count = 0; count < table_size; count++)
	{
		if (shaped_table[count].base == ch)
		{
			return &shaped_table[count];
		}
	}

	return NULL;
}

/* ------------------------- interface functions --------------------------- */

int
shape_n_join(
	FriBidiChar *str_visual, int str_len)
{
	int i;    /* counter of the input string */
	int len;  /* counter and the final length of the shaped string */
	FriBidiChar *orig_str;
	const char_shaped_t **list;
	const char_shaped_t *prev;
	const char_shaped_t *curr;
	const char_shaped_t *next;

	list = xmalloc((str_len + 2) * sizeof(char_shaped_t *));

	orig_str = xmalloc((str_len + 1) * sizeof(FriBidiChar));

	/* head is NULL */
	*list = NULL;
	list++;

	/* Populate with existent shaped characters */
	for (i = 0; i < str_len; i++)
	{
		list[i] = get_shaped_entry(str_visual[i]);
	}

	/* tail is NULL */
	list[i] = NULL;

	/* Store-off non-shaped characters; start with a clean slate */
	memcpy(orig_str, str_visual, (str_len * sizeof(str_visual[0])));
	memset(str_visual, 0, (str_len * sizeof(str_visual[0])));

	/* Traverse the line & build new content */
	for (i = 0, len = 0; i <= str_len - 1; i++, len++)
	{
		/* Get previous, current, and next characters */
		prev = list[i + 1];
		curr = list[i];
		next = list[i - 1];

		/* Process current mapped characters */
		if (curr)
		{
			if (next)
			{
				if (prev)
				{
					if (!prev->initial || !prev->medial)
					{
						str_visual[len] =
							curr->initial?
							curr->initial:
							curr->isolated;
					}
					else
					{
						str_visual[len] = curr->medial?
							curr->medial:
							curr->final;
					}
				}
				else
				{
					str_visual[len] = curr->initial?
						curr->initial:
						curr->isolated;
				}
			}
			else
			{
				if (prev)
				{
					if (!prev->initial || !prev->medial)
					{
						str_visual[len] =
							curr->isolated;
					}
					else
					{
						str_visual[len] = curr->final?
							curr->final:
							curr->isolated;
					}
				}
				else
				{
					str_visual[len] = curr->isolated;
				}
			}
		}
		else
		{
			str_visual[len] = orig_str[i];
		}
	}

	free(list-1);
	free(orig_str);

	/* return the length of the new string */
	return len;
}

#endif /* HAVE_BIDI */
