# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

package FVWM::EventNames;

use FVWM::Constants;
use vars qw($EVENTS_INFO);

$EVENTS_INFO = {

	&M_NEW_PAGE             => {
		format => "l5",
		names => [ qw(
			vp_x vp_y desk vp_width vp_height
		)],
	},

	&M_NEW_DESK             => {
		format => "l",
		names => [ qw(
			desk
		)],
	},

	&M_OLD_ADD_WINDOW       => {
		format => "l24",
		names => [ qw(
			obsolete
		)],
	},

	&M_RAISE_WINDOW         => {
		format => "l3",
		names => [ qw(
			win_id frame_id ptr
		)],
	},

	&M_LOWER_WINDOW         => {
		format => "l3",
		names => [ qw(
			win_id frame_id ptr
		)],
	},

	&M_OLD_CONFIGURE_WINDOW => {
		format => "l24",
		names => [ qw(
			obsolete
		)],
	},

	&M_FOCUS_CHANGE         => {
		format => "l5",
		names => [ qw(
			win_id frame_id flip focus_fg focus_bg
		)],
	},

	&M_DESTROY_WINDOW       => {
		format => "l3",
		names => [ qw(
			win_id frame_id ptr
		)],
	},

	&M_ICONIFY              => {
		format => [ "l7", "l11" ],
		names => [ qw(
			win_id frame_id ptr icon_x icon_y icon_width icon_height
			frame_x frame_y frame_width frame_height
		)],
	},

	&M_DEICONIFY            => {
		format => [ "l7", "l11" ],
		names => [ qw(
			win_id frame_id ptr icon_x icon_y icon_width icon_height
			frame_x frame_y frame_width frame_height
		)],
	},

	&M_WINDOW_NAME          => {
		format => "l3a*",
		names => [ qw(
			win_id frame_id ptr name
		)],
	},

	&M_ICON_NAME            => {
		format => "l3a*",
		names => [ qw(
			win_id frame_id ptr name
		)],
	},

	&M_RES_CLASS            => {
		format => "l3a*",
		names => [ qw(
			win_id frame_id ptr name
		)],
	},

	&M_RES_NAME             => {
		format => "l3a*",
		names => [ qw(
			win_id frame_id ptr name
		)],
	},

	&M_END_WINDOWLIST       => {
		format => "",
		names => [ qw(
		)],
	},

	&M_ICON_LOCATION        => {
		format => "l7",
		names => [ qw(
			win_id frame_id ptr x y width height
		)],
	},

	&M_MAP                  => {
		format => "l3",
		names => [ qw(
			win_id frame_id ptr
		)],
	},

	&M_ERROR                => {
		format => "l3a*",
		names => [ qw(
			win_id frame_id ptr text
		)],
	},

	&M_CONFIG_INFO          => {
		format => "l3a*",
		names => [ qw(
			zero1 zero2 zero3 line
		)],
	},

	&M_END_CONFIG_INFO      => {
		format => "",
		names => [ qw(
		)],
	},

	&M_ICON_FILE            => {
		format => "l3a*",
		names => [ qw(
			win_id frame_id ptr filename
		)],
	},

	&M_DEFAULTICON          => {
		format => "l3a*",
		names => [ qw(
			win_id frame_id ptr filename
		)],
	},

	&M_STRING               => {
		format => "l3a*",
		names => [ qw(
			win_id frame_id ptr line
		)],
	},

	&M_MINI_ICON            => {
		format => "l8a*",
		names => [ qw(
			win_id frame_id ptr width height depth icon_id mask
			alpha filename
		)],
	},

	&M_WINDOWSHADE          => {
		format => "l3",
		names => [ qw(
			win_id frame_id ptr
		)],
	},

	&M_DEWINDOWSHADE        => {
		format => "l3",
		names => [ qw(
			win_id frame_id ptr
		)],
	},

	&M_VISIBLE_NAME         => {
		format => "l3a*",
		names => [ qw(
			win_id frame_id ptr name
		)],
	},

#	&M_SENDCONFIG           => {
#	},

	&M_RESTACK              => {
		format => "l*",
		names => [ qw(
			win_id frame_id ptr name
		)],
	},

	&M_ADD_WINDOW           => {
		format => "l25",
		names => [ qw(
			win_id frame_id ptr frame_x frame_y frame_width
			frame_height desk layer title_height border_width
			win_width win_height resize_width_inc resize_height_inc
			minimum_width minimum_height maximum_width
			maximum_height icon_title_id icon_image_id gravity
			fore_color back_color style_flags
		)],
	},

	&M_CONFIGURE_WINDOW     => {
		format => "l25",
		names => [ qw(
			win_id frame_id ptr frame_x frame_y frame_width
			frame_height desk layer title_height border_width
			win_width win_height resize_width_inc resize_height_inc
			minimum_width minimum_height maximum_width
			maximum_height icon_title_id icon_image_id gravity
			fore_color back_color style_flags
		)],
	},

#	&M_EXTENDED_MSG         => {
#	},

	&MX_VISIBLE_ICON_NAME   => {
		format => "l3a*",
		names => [ qw(
			win_id frame_id ptr name
		)],
	},

	&MX_ENTER_WINDOW        => {
		format => "l3",
		names => [ qw(
			win_id frame_id ptr
		)],
	},

	&MX_LEAVE_WINDOW        => {
		format => "l3",
		names => [ qw(
			win_id frame_id ptr
		)],
	},

	&MX_PROPERTY_CHANGE     => {
		format => "l3a*",
		names => [ qw(
			property value win_id string
		)],
	},

	"faked"                 => {
		format => "",
		names => [ qw(
		)],
	},

};

=head1 NAME

FVWM::EventNames - names of all FVWM event arguments

=head1 SYNOPSIS

Please use L<FVWM::Event> instead.

=head1 DESCRIPTION

To be written.

=cut

1;
