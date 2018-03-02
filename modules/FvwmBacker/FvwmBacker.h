/* -*-c-*- */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#include "libs/fvwmlib.h"

/* Function Prototypes */

void EndLessLoop(void);
void ReadFvwmPipe(void);
void ProcessMessage(unsigned long type, unsigned long *body);
RETSIGTYPE DeadPipe(int nonsense);
void ParseConfig(void);
int ParseConfigLine(char *line);
void AddCommand(char *line);
