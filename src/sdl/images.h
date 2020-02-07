/* sdl/images.h: image resources.
 *
 * These functions provide access to a number of images which have
 * been compiled directly into the binary.
 */

#ifndef _sdl_images_h_
#define _sdl_images_h_

#include <SDL.h>
#include "./types.h"

/* Include the list of available image IDs, generated from the sprite
 * layout data. The IDs all begin with the "IMAGE_" prefix.
 */
#include "sdl/imageids.h"

/* Return the width of an image.
 */
extern int getimagewidth(int id);

/* Return the height of an image.
 */
extern int getimageheight(int id);

/* Load all of the interface's images into main memory.
 */
extern void initializeimages(void);

/* Extract the given image, returning a copy of its in its own
 * surface. Please note that this function can only be called after
 * initializeimages() and before finalizeimages().
 */
extern SDL_Surface *getimagesurface(int id);

/* Translate the program's images into textures, after which the
 * images will no longer be avaiable in main memory. This function
 * must be called before renderimage() or renderalphaimage().
 */
extern void finalizeimages(void);

/* Create the titlebar graphic texture for the opening display.
 */
extern SDL_Texture *loadsplashgraphic(void);

/* Create the titlebar headline texture for the opening display.
 */
extern SDL_Texture *loadsplashheadline(void);

/* Render an image texture at the given location on the display.
 */
extern void renderimage(int imageid, int x, int y);

/* Render an image texture at the given location, using alpha to blend
 * it with the current contents.
 */
extern void renderalphaimage(int imageid, int alpha, int x, int y);

/* Render a single playing card at the given location.
 */
extern void rendercard(card_t card, int x, int y);

#endif
