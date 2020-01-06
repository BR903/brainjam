/* ./ui.h: the API for the user interface.
 *
 * This file defines an API for user interface code. The program
 * provides two different user interfaces: a graphical interface using
 * SDL, and a text-based interface for an ANSI-compatible terminal.
 * Because each interface uses the same API, which interface to use
 * can be selected at runtime.
 */

#ifndef _ui_h_
#define _ui_h_

#include "./types.h"
#include "redo/types.h"

/* The full list of potential user interfaces.
 */
#define UI_NONE   0
#define UI_SDL    1
#define UI_CURSES 2
#define UI_ANSI   3

/* Select the user interface and initialize the interactive part of
 * the program. uimode indicates which interface to use. False is
 * returned if the user interace cannot be activated.
 */
extern int initializeui(int uimode);

/* Parameters to the rendergame() UI function.
 */
struct renderparams {
    gameplayinfo const *gameplay;       /* the state of the game */
    redo_position const *position;      /* the current redo position */
    int bookmark;                       /* true if a bookmark exists */
};

/* The set of functions that a user interface provides.
 */
typedef struct uimap {

    /* Display the game in its current state on the screen, along with
     * information about its position in the game tree.
     */
    void (*rendergame)(renderparams const *params);

    /* Wait for a command to be input from the user. The return value
     * is the command.
     */
    command_t (*getinput)(void);

    /* Artificially push a command onto the queue of input events, so
     * that cmd is returned from a subsequent call to getinput() after
     * a pause of msec milliseconds.
     */
    void (*ungetinput)(command_t cmd, int msec);

    /* Enable or disable card animations. The new setting is returned.
     */
    int (*setcardanimationflag)(int flag);

    /* Alert the user by ringing the terminal bell, flashing the
     * window, or some equivalent.
     */
    void (*ding)(void);

    /* Notify the user that a new solution has been saved.
     */
    void (*showsolutionwrite)(void);

    /* Animate the card currently at position from, having it slide
     * across the display to position to, and then invoking the given
     * callback after the animation completes. If animations are
     * currently disabled, this function will simply invoke the
     * callback directly and return.
     */
    void (*movecard)(card_t card, position_t from, position_t to,
                     void (*callback)(void*), void *data);

    /* Display the program's settings and allow the user to modify
     * them. A return value of true indicates that the changed
     * settings should be applied to the running program.
     */
    int (*changesettings)(settingsinfo *settings);

    /* Display all of the available configurations as a list and let
     * the user select one. The argument specifies the most recently
     * visited configuration. The return value is the ID of the
     * configuration that the user selected, or -1 if the user asked
     * to exit the program instead.
     */
    int (*selectconfig)(int currentconfig);

    /* Add information to the online help. title is the topic that the
     * contents of text explains. If placefirst is true, this topic is
     * placed before any existing topics; otherwise, it is placed at
     * the end of existing help topics.
     */
    void (*addhelpsection)(char const *title, char const *text,
                           int placefirst);

} uimap;

/* The active user interface global.
 */
extern uimap _ui;

/* Macros to simplify access to the functions of the user interface.
 */
#define rendergame(rp)               (_ui.rendergame(rp))
#define getinput()                   (_ui.getinput())
#define ungetinput(ch, ms)           (_ui.ungetinput(ch, ms))
#define setcardanimationflag(f)      (_ui.setcardanimationflag(f))
#define ding()                       (_ui.ding())
#define showsolutionwrite()          (_ui.showsolutionwrite())
#define movecard(cd, fr, to, cb, dt) (_ui.movecard(cd, fr, to, cb, dt))
#define changesettings(si)           (_ui.changesettings(si))
#define selectconfig(gp)             (_ui.selectconfig(gp))
#define addhelpsection(tl, tx, pf)   (_ui.addhelpsection(tl, tx, pf))

#endif