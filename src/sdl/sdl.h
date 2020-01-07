/* sdl/sdl.h: the graphical user interface, implemented using SDL.
 */

#ifndef _sdl_sdl_h_
#define _sdl_sdl_h_

#include "./ui.h"

/* Initialize the SDL user interface. If the interface cannot be
 * initialized, the returned structure will have the first function
 * pointer ("rendergame") set to NULL.
 */
extern uimap sdl_initializeui(void);

#endif
