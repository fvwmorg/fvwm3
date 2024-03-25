## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libWidgets.a

libWidgets_a_SOURCES = CheckBox.c HDipstick.c HScrollBar.c ItemDraw.c \
	List.c Menu.c MiniScroll.c PopupMenu.c PushButton.c \
	RadioButton.c Rectangle.c Swallow.c TextField.c Tools.c \
	Tools.h VDipstick.c VScrollBar.c Widget.c

AM_CPPFLAGS = -I$(top_srcdir) $(xpm_CFLAGS) $(Xft_CFLAGS) $(X_CFLAGS)

