/*
 * This program is free software; you can redistribute it and/or modify
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

typedef enum {
  READ_LINE = 1,
  READ_OPTION = 2,
  READ_ARG = 4,
  READ_REST_OF_LINE = 12
} ReadOption;

extern void read_in_resources (char *file);
extern void print_bindings (Binding *list);
extern void print_args (int numargs, BuiltinArg *args);
extern Binding *ParseMouseEntry (char *tline);

extern void run_function_list (Function *func);
extern void run_binding (WinManager *man, Action action);

#define MODS_USED (ShiftMask | ControlMask | Mod1Mask | \
		   Mod2Mask| Mod3Mask| Mod4Mask| Mod5Mask)

