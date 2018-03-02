/* -*-c-*- */
/*
 * FvwmButtons, copyright 1996, Jarl Totland
 *
 * This module, and the entire GoodStuff program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 */

/* --------------------------- button information -------------------------- */

void buttonInfo(
	const button_info *, int *x, int *y, int *padx, int *pady, int *frame);
void GetInternalSize(button_info *, int *, int *, int *, int *);
#define buttonFrame(b) abs(buttonFrameSigned(b))
int buttonFrameSigned(button_info *);
int buttonXPad(button_info *);
int buttonYPad(button_info *);
FlocaleFont *buttonFont(button_info *);
Pixel buttonFore(const button_info *);
Pixel buttonBack(const button_info *);
Pixel buttonHilite(button_info *);
Pixel buttonShadow(button_info *);
int buttonColorset(button_info *b);
char *buttonTitle (button_info *b);
FvwmPicture *buttonIcon (button_info *b);
unsigned short iconFlagSet (button_info *b);
int buttonBackgroundButton(button_info *b, button_info **r_b);
byte buttonSwallow(button_info *);
byte buttonJustify(button_info *);
#define buttonNum(b) ((b)->n)

/* ---------------------------- button creation ---------------------------- */

void alloc_buttonlist(button_info *, int);
button_info *alloc_button(button_info *, int);
void MakeContainer(button_info *);

/* ------------------------- button administration ------------------------- */

void NumberButtons(button_info *);
void ShuffleButtons(button_info *);

/* ---------------------------- button iterator ---------------------------- */

button_info *NextButton(button_info **, button_info **, int *, int);

/* --------------------------- button navigation --------------------------- */

int button_belongs_to(button_info *, int);
button_info *get_xy_button(button_info *ub, int row, int column);
button_info *select_button(button_info *, int, int);

/* --------------------------- button geometry ----------------------------- */

int buttonXPos(const button_info *b, int i);
int buttonYPos(const button_info *b, int i);
int buttonWidth(const button_info *b);
int buttonHeight(const button_info *b);
void get_button_root_geometry(rectangle *r, const button_info *b);

/* --------------------------- swallowing ---------------------------------- */

int buttonSwallowCount(button_info *b);
