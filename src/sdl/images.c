/* sdl/images.c: image resources.
 *
 * This file provides functions to access the static images that are
 * compiled into the binary and used to build the user interface.
 */

#include "SDL.h"
#include "./gen.h"
#include "./types.h"
#include "./incbin.h"
#include "internal.h"
#include "zrwops.h"
#include "images.h"

/* The first two images are used to create the title graphic for the
 * list display.
 */
INCBIN("sdl/banner.bmp.gz", gzbanner, gzbanner_end);
INCBIN("sdl/headline.bmp.gz", gzheadline, gzheadline_end);

/* The third image is a collection of smaller images, stored in the
 * fashion of a sprite sheet. The individual images are identified
 * using values defined in sdl/imageids.h, and are located via the
 * imagerects array defined below.
 */
INCBIN("sdl/images.bmp.gz", gzimages, gzimages_end);

/* The final image provides the complete set of playing cards.
 *
 * The playing cards are laid out in a 16x4 array. Each of the four
 * rows contains all the cards for one suit. The first two columns
 * contain special graphics. The actual cards begin with the aces in
 * the third column, then the twos, and so on up to the kings in the
 * next-to-last column.
 *
 * The contents of the first column, going from top to bottom, are the
 * index marker graphic, an "empty" image indicating a location
 * without a card, a mask for the empty image, and a blank card. The
 * index marker graphic is not an actual image, but rather a black
 * image with red pixels that indicate the position of the cards'
 * corner index labels.
 *
 * The second column contains four "empty" card images that indicate
 * the suit required for cards played there, such as appear at an
 * empty foundation pile.
 *
 * (The final column contains two jokers and two card backs, neither
 * of which are used by this program.)
 */
INCBIN("sdl/cardset.bmp.gz", gzcardset, gzcardset_end);

/* The texture containing the cards.
 */
static SDL_Texture *decktexture;

/* The locations of each card in the texture.
 */
static SDL_Rect cardrects[64];

/* The "sheet" of images. The SDL surface is used during
 * initialization. After initialization is complete, the images are
 * moved from the surface to the texture.
 */
static SDL_Surface *imagesurface = NULL;
static SDL_Texture *imagetexture = NULL;

/* The array containing the position and size of each separate
 * sub-image in the image resource. The actual values are stored in a
 * separate header file, generated by the sprite layout data, and so
 * need to be included into the middle of the variable initialization
 * statement.
 */
static SDL_Rect const imagerects[IMAGE_COUNT] = {
#include "sdl/layout.h"
};

/* Examine the pixels inside a rectangle in the top-left corner of the
 * image, with the given size, and locate the red pixels. (These
 * pixels mark the location of the index of the cards.) Return the
 * y-coordinate of the first line of pixels below the red shape.
 */
static int finddropheight(SDL_Surface *image, SDL_Point size)
{
    Uint8 *pixels, *p;
    Uint32 value;
    int height;
    int x, y;

    if (SDL_MUSTLOCK(image))
        SDL_LockSurface(image);
    pixels = image->pixels;

    height = size.y / 2;
    if (image->format->BitsPerPixel == 8) {
        value = SDL_MapRGB(image->format, 255, 0, 0);
        for (y = 0 ; y < size.y / 2 ; ++y) {
            for (x = 0 ; x < size.x ; ++x) {
                if (pixels[x] == value) {
                    height = y;
                    break;
                }
            }
            pixels += image->pitch;
        }
    } else {
        for (y = 0 ; y < size.y / 2 ; ++y) {
            p = pixels;
            for (x = 0 ; x < size.x ; ++x) {
                switch (image->format->BitsPerPixel) {
                  case 16:
                    value = *(Uint16*)p;
                    p += 2;
                    break;
                  case 32:
                    value = *(Uint32*)p;
                    p += 4;
                    break;
                  case 24:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                    value = *(Uint32*)pixels >> 8;
#else
                    value = *(Uint32*)pixels & 0x00FFFFFF;
#endif
                    p += 3;
                    break;
                  default:
                    value = *p++;
                    break;
                }
                if ((value & image->format->Rmask) == image->format->Rmask) {
                    height = y;
                    break;
                }
            }
            pixels += image->pitch;
        }
    }

    return height + 1;
}

/* Create the texture holding the playing card images, and locate each
 * card within the texture.
 */
static void initdeck(void)
{
    SDL_Surface *image;
    int i;

    image = SDL_LoadBMP_RW(getzresource(gzcardset), TRUE);
    decktexture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_QueryTexture(decktexture, NULL, NULL,
                     &_graph.cardsize.x, &_graph.cardsize.y);
    _graph.cardsize.x /= 16;
    _graph.cardsize.y /= 4;
    _graph.dropheight = finddropheight(image, _graph.cardsize);
    SDL_FreeSurface(image);
    for (i = 0 ; i < 64 ; ++i) {
        cardrects[i].x = (i / 4) * _graph.cardsize.x;
        cardrects[i].y = (i % 4) * _graph.cardsize.y;
        cardrects[i].w = _graph.cardsize.x;
        cardrects[i].h = _graph.cardsize.y;
    }
}

/*
 * Internal functions.
 */

/* Return the width of the given image.
 */
int getimagewidth(int id)
{
    return imagerects[id].w;
}

/* Return the height of the given image.
 */
int getimageheight(int id)
{
    return imagerects[id].h;
}

/* Load the playing cards and the image sheet.
 */
void initializeimages(void)
{
    initdeck();
    imagesurface = SDL_LoadBMP_RW(getzresource(gzimages), TRUE);
    imagetexture = NULL;
}

/* Create a separate copy of one image from the sprite sheet.
 */
SDL_Surface *getimagesurface(int id)
{
    SDL_Surface *image;

    image = SDL_CreateRGBSurface(0, imagerects[id].w, imagerects[id].h,
                                 imagesurface->format->BitsPerPixel,
                                 imagesurface->format->Rmask,
                                 imagesurface->format->Gmask,
                                 imagesurface->format->Bmask,
                                 imagesurface->format->Amask);
    SDL_BlitSurface(imagesurface, &imagerects[id], image, NULL);
    return image;
}

/* Translate the sprite sheet image into a texture.
 */
void finalizeimages(void)
{
    imagetexture = SDL_CreateTextureFromSurface(_graph.renderer, imagesurface);
    SDL_FreeSurface(imagesurface);
    imagesurface = NULL;
}

/* Return the splash screen's banner graphic image as a texture.
 */
SDL_Texture *loadsplashgraphic(void)
{
    SDL_Surface *image;
    SDL_Texture *texture;

    image = SDL_LoadBMP_RW(getzresource(gzbanner), TRUE);
    texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(image);
    return texture;
}

/* Return the splash screen's banner headline image as a texture.
 */
SDL_Texture *loadsplashheadline(void)
{
    SDL_Surface *image;
    SDL_Texture *texture;

    image = SDL_LoadBMP_RW(getzresource(gzheadline), TRUE);
    texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(image);
    return texture;
}

/* Render the given card at the given location.
 */
void rendercard(card_t card, int x, int y)
{
    SDL_Rect rect;

    rect.x = x;
    rect.y = y;
    rect.w = cardrects->w;
    rect.h = cardrects->h;
    SDL_RenderCopy(_graph.renderer, decktexture, &cardrects[card], &rect);
}

/* Render an image at the given location.
 */
void renderimage(int id, int x, int y)
{
    SDL_Rect rect;

    rect.x = x;
    rect.y = y;
    rect.w = imagerects[id].w;
    rect.h = imagerects[id].h;
    SDL_RenderCopy(_graph.renderer, imagetexture, &imagerects[id], &rect);
}

/* Render an image with the given alpha blending value.
 */
void renderalphaimage(int id, int alpha, int x, int y)
{
    SDL_SetTextureAlphaMod(imagetexture, alpha);
    renderimage(id, x, y);
    SDL_SetTextureAlphaMod(imagetexture, SDL_ALPHA_OPAQUE);
}
