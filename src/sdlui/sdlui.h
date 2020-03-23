/* sdlui/sdlui.h: the graphical user interface, implemented using SDL.
 */

#ifndef _sdlui_sdlui_h_
#define _sdlui_sdlui_h_

#include "./ui.h"

/* Initialize the SDL user interface. If the interface cannot be
 * initialized, the returned structure will have the first function
 * pointer ("rendergame") set to NULL.
 */
extern uimap sdlui_initializeui(void);

#endif
