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

#ifndef CONDITIONAL_H
#define CONDITIONAL_H

/* Condition matching routines
 * Originally exported for WindowList - N.Bird 24/08/99
 */
extern char *CreateFlagString(char *string, char **restptr);
extern void DefaultConditionMask(WindowConditionMask *mask);
extern void CreateConditionMask(char *flags, WindowConditionMask *mask);
extern void FreeConditionMask(WindowConditionMask *mask);
extern Bool MatchesConditionMask(FvwmWindow *fw, WindowConditionMask *mask);

#endif /* CONDITIONAL_H */
