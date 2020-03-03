/* sdl/images.h: image resources.
 *
 * These functions provide access to a number of images which have
 * been compiled directly into the binary.
 */

#ifndef _sdl_images_h_
#define _sdl_images_h_

#include <SDL.h>
#include "./types.h"

/* Include the lists of available image IDs, generated from the sprite
 * layout data. The IDs all begin with the "IMAGE_" prefix.
 */
#include "sdl/alertids.h"
#include "sdl/biconids.h"

/* Return the width of an alert image.
 */
extern int getimagewidth(int id);

/* Return the height of an alert image.
 */
extern int getimageheight(int id);

/* Load all of the interface's graphics.
 */
extern void initializeimages(void);

/* Return a copy of a button graphic. Note that this function can only
 * be called after initializeimages() and before finalizeimages().
 */
extern SDL_Surface *getbuttonicon(int id);

/* Deallocate image surfaces needed only during initialization.
 */
extern void finalizeimages(void);

/* Create the titlebar graphics for the opening display and return
 * them as two separate textures.
 */
extern void loadsplashgraphics(SDL_Texture **pbanner, SDL_Texture **pheadline);

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
