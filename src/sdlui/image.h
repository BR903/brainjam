/* sdlui/image.h: image resources.
 *
 * These functions provide access the static images that are
 * compiled into the binary and used to build the user interface.
 */

#ifndef _sdlui_image_h_
#define _sdlui_image_h_

#include <SDL.h>
#include "./types.h"

/* Include the lists of available image IDs, generated from the sprite
 * layout data. The IDs all begin with the "IMAGE_" prefix.
 */
#include "sdlui/alertids.h"
#include "sdlui/labelids.h"

/* Load all of the interface's graphics.
 */
extern void initializeimages(void);

/* Return a copy of a button graphic. Note that this function can only
 * be called after initializeimages() and before finalizeimages().
 */
extern SDL_Surface *getbuttonlabel(int id);

/* Deallocate the button icon images, as they should all have been
 * copied to the button textures after initialization is completed.
 */
extern void finalizeimages(void);

/* Load the titlebar graphics for the opening display and return them
 * as two separate textures.
 */
extern void loadsplashgraphics(SDL_Texture **pbanner, SDL_Texture **pheadline);

/* Return the width of an alert image.
 */
extern int getimagewidth(int id);

/* Return the height of an alert image.
 */
extern int getimageheight(int id);

/* Render an alert image at the given location on the display.
 */
extern void renderimage(int imageid, int x, int y);

/* Render an alert image at the given location, using alpha to blend
 * it with the current contents.
 */
extern void renderalphaimage(int imageid, int alpha, int x, int y);

/* Render a single playing card at the given location.
 */
extern void rendercard(card_t card, int x, int y);

#endif
