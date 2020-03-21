/* sdl/internal.h: All functions and data internal to the SDL UI module.
 */

#ifndef _sdl_internal_h_
#define _sdl_internal_h_

#include <SDL.h>
#include <SDL_ttf.h>
#include "./types.h"
#include "redo/redo.h"

/* SDL is not entirely consistent about using the SDL_Color struct;
 * very often its API calls for separate integer arguments. These
 * macros help reduce verbosity in this case, breaking out an
 * SDL_Color struct into multiple arguments.
 */
#define colors3(c)  (c).r, (c).g, (c).b
#define colors4(c)  (c).r, (c).g, (c).b, (c).a

/* The list of displays that the game provides.
 */
#define DISPLAY_NONE    0       /* no active display */
#define DISPLAY_LIST    1       /* the game list and splash screen */
#define DISPLAY_GAME    2       /* the game proper */
#define DISPLAY_HELP    3       /* the help screens */
#define DISPLAY_COUNT   4

/* Forward declaration of a struct defined in button.h. (There must be
 * a nicer way to handle this, short of having to merge button.h with
 * this file.)
 */
struct button;

/* Data describing moving (or otherwise changing) display values. An
 * animation is one or two integer values that are periodically
 * modified until they reach their destination values. (An animation
 * that only has one changing value should set the other integer
 * pointer to NULL.) An optional callback can be provided, which will
 * be invoked after the animation completes but before the animinfo is
 * freed.
 */
typedef struct animinfo {
    int         inuse;          /* true while the struct is being used */
    int         steps;          /* how many ticks the change should take */
    int        *pval1;          /* first changing value */
    int        *pval2;          /* second changing value */
    int         destval1;       /* first value to end on */
    int         destval2;       /* second value to end on */
    void      (*callback)(void*,int); /* callback to invoke at end */
    void       *data;           /* argument to pass to the callback */
} animinfo;

/* The set of three callbacks that provide the functionality of one of
 * the game's displays. These callbacks are invoked from the event loop
 * at the appropriate times.
 */
typedef struct displaymap {

    /* Determine the layout of the display's elements. size provides
     * the size of the window. The return value should specify the
     * display's ideal minimum size, one that still allows the display
     * to function comfortably. If the returned size is larger than
     * the current size, the window may be grown to accommodate this
     * minimum. As such, this function can be called multiple times
     * during a window resize, so its cost should be kept light. Note
     * that this function, unlike the others, can be called when this
     * display is not active.
     */
    SDL_Point (*setlayout)(SDL_Point size);

    /* Draw the current display to the SDL renderer. Upon return, the
     * program will overlay any visible buttons before presenting the
     * display to the window. This callback is invoked when the
     * display becomes active, when the window is exposed or resized,
     * and whenever there is a change of state.
     */
    void (*render)(void);

    /* Handle an I/O event. If the return value is cmd_none (i.e.
     * zero), then the event is handled by the default processing
     * shared by all displays. Otherwise, it is assumed that the event
     * was fully processed by this function, and the return value is
     * passed back up from the current getinput() API call. (A return
     * value of cmd_redraw can be used as a command that does not
     * trigger further state changes but still causes the display to
     * be udpated.)
     */
    command_t (*eventhandler)(SDL_Event *event);

} displaymap;

/* Shared objects and data used throughout this module.
 */
typedef struct graphicinfo {

    /* The SDL renderer, which providess access to the actual display.
     * Most SDL rendering functions require this as the first
     * parameter.
     */
    SDL_Renderer *renderer;

    /* The SDL_ttf fonts, which are used for all text output.
     */
    TTF_Font *smallfont;
    TTF_Font *largefont;

    /* The set of colors used for rendering text: regular text, dimmed
     * text, bolded and highlighted text, and finally text background
     * and highlighted text background.
     */
    SDL_Color defaultcolor;
    SDL_Color dimmedcolor;
    SDL_Color highlightcolor;
    SDL_Color bkgndcolor;
    SDL_Color lightbkgndcolor;

    /* A unit of spacing measurement, which is based on the size of
     * the text font (which in turn is based on the size of the
     * playing card images). This value provides a rough grid on which
     * the display layouts are aligned.
     */
    int margin;

    /* The width and height of a playing card image.
     */
    SDL_Point cardsize;

    /* The vertical distance necessary to avoid obfuscating a card's
     * index when another card is placed over it.
     */
    int dropheight;

} graphicinfo;

/* The graphic rendering information global, defined in sdl.c
 */
extern graphicinfo _graph;

/*
 * Functions defined in sdl.c.
 */

/* Return true if the point (x, y) is within the given rectangle.
 */
extern int rectcontains(SDL_Rect const *rect, int x, int y);

/* Register a button. The button will be automatically displayed, and
 * mouse events involving the button will be manged and generate user
 * commands or action callbacks as appropriate.
 */
extern void addbutton(struct button *btn);

/* Render a string of text using the given font and position. The
 * align parameter should be positive to render the text to the right
 * of the position, negative to render the text to the left of the
 * position, or zero to center the text at the position.
 */
extern int drawtext(char const *string, int x, int y, int align,
                    TTF_Font *font);

/* Render a decimal number using the given font and position.
 */
extern int drawnumber(int number, int x, int y, int align, TTF_Font *font);

/* Macros to supply the font to the text-rendering functions.
 */
#define drawsmalltext(str, x, y, a) drawtext(str, x, y, a, _graph.smallfont)
#define drawlargetext(str, x, y, a) drawtext(str, x, y, a, _graph.largefont)
#define drawsmallnumber(n, x, y, a) drawnumber(n, x, y, a, _graph.smallfont)
#define drawlargenumber(n, x, y, a) drawnumber(n, x, y, a, _graph.largefont)

/* Set the color used to render text in subsequent calls to the
 * above functions.
 */
extern void settextcolor(SDL_Color color);

/* Return a pointer to an available animinfo struct.
 */
extern animinfo *getaniminfo(void);

/* Begin the process of animating a display element. The values
 * indicated by the animinfo fields will be updated, every msec
 * milliseconds, until they reach their final values, or until
 * stopanimation() is called.
 */
extern void startanimation(animinfo *anim, int msec);

/* Stop displaying the given animation. If finish is true, then the
 * values being altered are set to their final state directly. If
 * finish is false, they are left at their current intermediate
 * values.
 */
extern void stopanimation(animinfo *anim, int finish);

/*
 * Functions defined in list.c.
 */

/* Create the list display and return its displaymap.
 */
extern displaymap initlistdisplay(void);

/* Set the list display to show the given game ID as the current
 * selection when it is next active.
 */
extern void initlistselection(int gameid);

/* Return the game ID that is currently selected.
 */
extern int getselection(void);

/*
 * Functions defined in game.c.
 */

/* Create the game display and return its displaymap.
 */
extern displaymap initgamedisplay(void);

/* Prepare the game display to become the active display. The displays
 * are not normally notified before being activated or deactivated.
 * However, the game display has persistent data that needs to be
 * reset when a new game is selected.
 */
extern void resetgamedisplay(void);

/* Show or hide the options popup dialog on the game display. If
 * display is true, then the checkboxes in the dialog are initialized
 * from the given settings. If display is false, however, then the
 * checkbox values are copied into the given settings.
 */
extern void showoptions(settingsinfo *settings, int display);

/* Update the most recent game state. The arguments provide all the
 * information necessary to correctly render the game display. (This
 * "side channel" is necessary because the render() displaymap
 * function takes no arguments.)
 */
extern void updategamestate(gameplayinfo const *gameplay,
                            redo_position const *position, int bookmark);

/*
 * Functions defined in help.c.
 */

/* Create the help display and return its displaymap.
 */
extern displaymap inithelpdisplay(void);

/*
 * The API functions. (The API functions not declared here are defined
 * inside of sdl.c.) See ./ui.h for descriptions of these functions.
 */

/* Defined in game.c.
 */
extern int sdl_setshowkeyguidesflag(int flag);
extern int sdl_setcardanimationflag(int flag);
extern void sdl_movecard(gameplayinfo const *gameplay, card_t card,
                         place_t from, place_t to,
                         void (*callback)(void*), void *data);
extern void sdl_showsolutionwrite(void);

/* Defined in help.c.
 */
extern void sdl_addhelpsection(char const *title, char const *text,
                               int placefirst);

#endif
