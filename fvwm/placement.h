/* -*-c-*- */

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
	const exec_context_t *exc, style_flags *sflags, rectangle *attr_g,
	int Desk, int PageX, int PageY, int XineramaScreen, int mode,
	initial_window_options_t *win_opts);

#endif /* PLACEMENT_H */
