/* sdlui/button.h: button creation.
 *
 * These functions are used to create the graphic representation for a
 * button object. Once this is done the button can be handed off to
 * the addbutton() function, which will manage its behavior
 * automatically.
 */

#ifndef _sdlui_button_h_
#define _sdlui_button_h_

#include <SDL.h>
#include "sdltypes.h"

/* The button state flags. A button's state value is a bitwise-or of
 * zero or more flags.
 */
#define BSTATE_NORMAL  0        /* default state */
#define BSTATE_HOVER   1        /* mouse is over the button */
#define BSTATE_DOWN    2        /* button is held down */
#define BSTATE_SELECT  4        /* button is selected */

/* Because in practice a button cannot be pushed down without also
 * having the mouse hovering, the state of down-on-hover-off is
 * repurposed to indicate a disabled button.
 */
#define BSTATE_DISABLED  BSTATE_DOWN

/* The data needed to display and monitor interaction with buttons.
 * After creating a button, the program should fill in the display
 * value, the pos's x and y values, the cmd value, and optionally the
 * action callback.
 */
struct button {
    SDL_Rect    pos;            /* location and size of the button */
    int         display;        /* which display the button appears on */
    command_t   cmd;            /* the key command the button generates */
    int         visible;        /* true if the button should be shown */
    int         state;          /* the button's current state */
    void      (*action)(int);   /* optional callback invoked on selection */
    SDL_Texture *texture;       /* appearance of the button for each state */
    SDL_Rect   *rects;          /* map of states to positions in the texture */
};

/* Create a pushbutton labeled with an image.
 */
extern void makeimagebutton(button *pushbutton, int iconid);

/* Create a checkbox button with a text label on the right.
 */
extern void makecheckbox(button *chkbox, char const *text);

/* Create a very specialized "popup" pushbutton, which permits an
 * object to be visually attached to its left-hand side when the
 * button is selected.
 */
extern void makepopupbutton(button *popup, int iconid);

#endif
