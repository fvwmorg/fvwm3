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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PLACEMENT_H
#define PLACEMENT_H

#define PLACE_INITIAL 0x0
#define PLACE_AGAIN   0x1

#define NORMAL_PLACEMENT_PENALTY(fw)      (fw->placement_penalty[0])
#define ONTOP_PLACEMENT_PENALTY(fw)       (fw->placement_penalty[1])
#define ICON_PLACEMENT_PENALTY(fw)        (fw->placement_penalty[2])
#define STICKY_PLACEMENT_PENALTY(fw)      (fw->placement_penalty[3])
#define BELOW_PLACEMENT_PENALTY(fw)       (fw->placement_penalty[4])
#define EWMH_STRUT_PLACEMENT_PENALTY(fw)  (fw->placement_penalty[5])

#define PERCENTAGE_99_PENALTY(fw) (fw->placement_percentage_penalty[0])
#define PERCENTAGE_95_PENALTY(fw) (fw->placement_percentage_penalty[1])
#define PERCENTAGE_85_PENALTY(fw) (fw->placement_percentage_penalty[2])
#define PERCENTAGE_75_PENALTY(fw) (fw->placement_percentage_penalty[3])

Bool PlaceWindow(
	FvwmWindow *fw, style_flags *sflags, rectangle *attr_g,
	int Desk, int PageX, int PageY, int XineramaScreen, int mode,
	initial_window_options_type *win_opts);

#endif /* PLACEMENT_H */
