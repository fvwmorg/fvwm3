#ifndef _DIALOG_H
#define _DIALOG_H

void open_dialog (int argc, char **argv);
void dialog_start_box (int argc, char **argv);
void dialog_start_frame (int argc, char **argv);
void dialog_end_something (int argc, char **argv);
void dialog_label (int argc, char **argv);
void dialog_notebook (int argc, char **argv);
void dialog_end_notebook (int argc, char **argv);
void dialog_button (int argc, char **argv);
void dialog_checkbutton (int argc, char **argv);
void dialog_entry (int argc, char **argv);
void dialog_radiobutton (int argc, char **argv);
void dialog_start_radiogroup (int argc, char **argv);
void dialog_end_radiogroup (int argc, char **argv);
void dialog_color (int argc, char **argv);
void dialog_separator (int argc, char **argv);
void dialog_scale (int argc, char **argv);
void dialog_spinbutton (int argc, char **argv);
void dialog_start_option_menu (int argc, char **argv);
void dialog_end_option_menu (int argc, char **argv);
void dialog_option_menu_item (int argc, char **argv);

#endif /* _DIALOG_H */
