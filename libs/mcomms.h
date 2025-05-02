#ifndef FVWMLIB_MCOMMS_H
#define FVWMLIB_MCOMMS_H

#include <stdbool.h>

#define MCOMMS_NEW_WINDOW	0x0000000000000001ULL
#define MCOMMS_ALL		0xffffffffffffffffULL

struct m_add_window {
	char	*window;
	int	 title_height;
	int	 border_width;

	struct {
		int	window;
		int	x;
		int	y;
		int	width;
		int	height;
	} frame;

	struct {
		int	base_width;
		int	base_height;
		int	inc_width;
		int	inc_height;
		int	orig_inc_width;
		int	orig_inc_height;
		int	min_width;
		int	min_height;
		int	max_width;
		int	max_height;
	} hints;

	struct {
		int	layer;
		int	desktop;
		int	window_type;
	} ewmh;
};

uint64_t	m_register_interest(int *, const char *);

#endif
