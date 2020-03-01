/* sdl/getpng.h: reading PNG file data.
 *
 * This function wraps the libpng API to transfer pixel data from an
 * in-memory PNG file to an SDL object.
 */

#ifndef _sdl_getpng_h_
#define _sdl_getpng_h_

#include <SDL.h>

/* Load an image from an internal resource to an SDL surface.
 */
extern SDL_Surface *pngtosurface(void const *pngdata);

#endif
