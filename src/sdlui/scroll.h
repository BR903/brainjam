/* sdlui/scroll.h: managing a scrollbar element.
 *
 * These functions encapsulate the management of a simple scrollbar
 * object, both the job of rendering it as well as dealing with the
 * various mouse events that interact with the scrollbar. (Keyboard
 * events are not handled by this code, and must be implemented
 * separately.)
 */

#ifndef _sdlui_scroll_h_
#define _sdlui_scroll_h_

#include <SDL.h>
#include "sdltypes.h"

/* The data used to manage a scrollbar on the display. (To disable a
 * scrollbar, set the range field to 0.)
 */
struct scrollbar {
    SDL_Rect    pos;            /* location and size of the scrollbar */
    int         value;          /* the current value of the scrollbar */
    int         range;          /* the set of possible values [0..range-1] */
    int         linesize;       /* the size of a line (for wheel events) */
    int         pagesize;       /* the size of a page (controls thumb size) */
};

/* Render a scrollbar to the display.
 */
extern void scrollrender(scrollbar const *scroll);

/* Handle an I/O event that might interact with the given scrollbar.
 * The return value is true if the event was handled by this function,
 * in which case the event should not be processed further.
 */
extern int scrolleventhandler(SDL_Event *event, scrollbar *scroll);

#endif
