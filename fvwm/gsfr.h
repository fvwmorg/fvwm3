#ifndef _GSFR_
#define _GSFR_

#include "fvwm.h"

typedef struct
{
  /* common flags (former flags in bits 0-12) */
  unsigned circulate_skip : 1; /* was circulateskip */
  unsigned circulate_skip_icon : 1;
#define FOCUS_MOUSE   0x0
#define FOCUS_CLICK   0x1
#define FOCUS_SLOPPY  0x2
  unsigned focus_mode : 2; /* was click_focus/sloppy_focus */
  unsigned lenience : 1;
  unsigned has_no_icon_title : 1; /* was noicon_title */
  unsigned show_on_map : 1; /* was show_mapping */
  unsigned start_iconic : 1;
  unsigned is_sticky : 1; /* was sticky */
  unsigned is_icon_sticky : 1; /* was sticky_icon */
  unsigned suppress_icon : 1; /* was suppressicon */
  unsigned window_list_skip : 1; /* was listskip */
} common_flags_type;

typedef struct
{
  common_flags_type common_flags;
  unsigned does_wm_take_focus : 1; /* DoesWmTakeFocus */
  unsigned does_wm_delete_window : 1; /* DoesWmDeleteWindow */
  unsigned has_border : 1; /* BORDER */ /* Is this decorated with border*/
  unsigned has_mwm_borders : 1; /* MWMBorders */
  unsigned has_mwm_buttons : 1; /* MWMButtons */
  unsigned has_title : 1; /* TITLE */ /* Is this decorated with title */
  unsigned hint_override : 1; /* HintOverride */ /**/
  unsigned is_mapped : 1; /* MAPPED */ /* is it mapped? */
  unsigned is_iconified : 1; /* ICONIFIED */ /* is it an icon now? */
  unsigned is_iconified_by_parent : 1; /* To prevent iconified transients in a parent
					* icon from counting for Next */
  unsigned is_icon_ours : 1; /* ICON_OURS */ /* is the icon window supplied by the app? */
  unsigned is_icon_shaped : 1; /* SHAPED_ICON */ /* is the icon shaped? */
  unsigned is_icon_moved : 1; /* ICON_MOVED */ /* has the icon been moved by the user? */
  unsigned is_icon_unmapped : 1; /* ICON_UNMAPPED */ /* was the icon unmapped, even though the window is still iconified (Transients) */
  unsigned is_map_pending : 1; /* MAP_PENDING */ /* Sent an XMapWindow, but didn't receive a MapNotify yet.*/
  unsigned is_maximized : 1; /* MAXIMIZED */ /* is the window maximized? */
  unsigned is_name_changed : 1; /* Set if the client changes its WM_NAME.
				 * The source of twm contains an explanation
				 * why we need this information. */
  unsigned is_pixmap_ours : 1; /* PIXMAP_OURS */ /* is the icon pixmap ours to free? */
  unsigned is_transient : 1; /* TRANSIENT */ /* is it a transient window? */
  unsigned is_viewport_moved : 1; /* To prevent double move in MoveViewport.*/
  unsigned is_visible : 1; /* VISIBLE */ /* is the window fully visible */
  unsigned is_window_being_moved_opaque : 1;
  unsigned is_window_shaded : 1;
  unsigned reuse_destroyed : 1;     /* Reuse this struct, don't free it,
				     * when destroying/recapturing window. */
} window_flags;

typedef struct
{
  common_flags_type common_flags;
  unsigned bw : 1; /* BW_FLAG */
  unsigned nobw : 1; /* NOBW_FLAG */
  unsigned color_fore : 1; /* FORE_COLOR_FLAG */
  unsigned color_back : 1; /* BACK_COLOR_FLAG */
  unsigned decorate_transient : 1; /* DECORATE_TRANSIENT_FLAG */
  unsigned grab_focus_when_created : 1; /* was grab_focus */
  unsigned has_no_title : 1; /* NOTITLE_FLAG */
  unsigned has_no_border : 1; /* NOBORDER_FLAG */
  unsigned has_icon : 1; /* ICON_FLAG */
#ifdef MINI_ICONS
  unsigned mini_icon : 1; /* MINIICON_FLAG */
#endif
  unsigned mwm_button : 1; /* MWM_BUTTON_FLAG */
  unsigned mwm_decor : 1; /* MWM_DECOR_FLAG */
  unsigned mwm_functions : 1; /* MWM_FUNCTIONS_FLAG */
  unsigned mwm_override : 1; /* MWM_OVERRIDE_FLAG */
  unsigned mwm_border : 1; /* MWM_BORDER_FLAG */
  unsigned no_pposition : 1; /* NO_PPOSITION_FLAG */
  unsigned ol_decor : 1; /* OL_DECOR_FLAG */
  unsigned place_random : 1; /* RANDOM_PLACE_FLAG */
  unsigned place_smart : 1; /* SMART_PLACE_FLAG */
  unsigned start_lowered : 1; /* starts_lowered */
  unsigned use_start_lowered : 1; /* has_starts_lowered */ /* has starts_lowered been set ? */
  unsigned use_start_on_desk : 1; /* STARTSONDESK_FLAG */
  unsigned use_layer : 1; /* has_layer */ /* has layer been set explicitly ? */
} style_flags;

typedef struct
{
  style_flags on_flags;
  style_flags off_flags;
  unsigned long on_buttons;
  unsigned long off_buttons;
  int border_width;
  int layer;
  int handle_width; /* resize_width */
  int start_desk; /* Desk */
  int start_page_x; /* PageX */
  int start_page_y; /* PageY */
  char *ForeColor;
  char *BackColor;
  char *icon_name; /* value */               /* icon name */
#ifdef MINI_ICONS
  char *mini_icon_name; /* mini_value */               /* mini icon name */
#endif
#ifdef USEDECOR
  char *decor_name; /* Decor */
#endif
  icon_boxes *IconBoxes;                /* pointer to iconbox(s) */
} window_style;

#endif /* _GSFR_ */
