/* cursesui/internal.h: internal functions of the curses UI module.
 */

#ifndef _cursesui_internal_h_
#define _cursesui_internal_h_

#include "./types.h"
#include "redo/redo.h"

/* The list of text "modes" used by the program. Each mode corresponds
 * to a different set of text attributes, determined at startup.
 */
#define MODEID_NORMAL     0             /* plain text */
#define MODEID_SELECTED   1             /* a selected element */
#define MODEID_HIGHLIGHT  2             /* a highlighted element */
#define MODEID_DARKER     3             /* a somewhat darkened element */
#define MODEID_DIMMED     4             /* a disabled or unselected element */
#define MODEID_TITLE      5             /* title text */
#define MODEID_BLACKCARD  6             /* text representing a black card */
#define MODEID_REDCARD    7             /* text representing a red card */
#define MODEID_FOUNDATION 8             /* an empty foundation */
#define MODEID_RESERVE    9             /* an empty reserve */
#define MODEID_COUNT      10

/* Change the mode of subsequent text.
 */
extern void textmode(int attrid);

/* Check the size of the terminal. If it is not at least 80x24,
 * display an error message and return false. This function should be
 * called just before each render, and if it returns false no further
 * update needs to be made to the display.
 */
extern int validatesize(void);

/* Wait for a keypress. The return value is the key that was pressed.
 */
extern int getkey(void);

/* Manage the help interface, allowing the user to browse through the
 * various topics. If title is not NULL, it should match the title of
 * the help section to display first. The function returns when the
 * user is done reading, and false is returned if the user asked to
 * exit the program.
 */
extern int runhelp(char const *title);

/*
 * The API functions. (Three API functions are defined statically in
 * cursesui.c itself, and so do not need to be declared here.) For
 * more information on these functions, see ./ui.h.
 */

/* Defined in game.c.
 */
extern void cursesui_rendergame(renderparams const *params);
extern command_t cursesui_getinput(void);
extern int cursesui_setshowkeyguidesflag(int flag);
extern int cursesui_setcardanimationflag(int flag);
extern void cursesui_movecard(gameplayinfo const *gameplay, card_t card,
                              place_t from, place_t to,
                              void (*callback)(void*), void *data);
extern void cursesui_showwriteindicator(void);

/* Defined in list.c.
 */
extern int cursesui_selectgame(int currentgameid);

/* Defined in help.c.
 */
extern void cursesui_addhelpsection(char const *title, char const *text,
                                    int putfirst);

#endif
