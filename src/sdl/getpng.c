/* sdl/getpng.c: reading PNGs file data.
 */

#include <stdlib.h>
#include <png.h>
#include <SDL.h>
#include "./gen.h"
#include "getpng.h"

/* The data that we keep on hand when reading a PNG file.
 */
typedef struct pnginfo {
    png_struct *png;
    png_info *info;
    int width;
    int height;
    png_byte **rows;
} pnginfo;

/* A memory buffer being treated as a stream needs to track its own
 * offset.
 */
typedef struct memstreaminfo {
    unsigned char const *data;
    off_t offset;
} memstreaminfo;

/* Callback for feeding data to libpng.
 */
static void readcallback(png_struct *png, png_byte *outbuf, png_size_t size)
{
    memstreaminfo *ms;

    ms = png_get_io_ptr(png);
    memcpy(outbuf, ms->data + ms->offset, size);
    ms->offset += size;
}

/* Import PNG data into the given pnginfo struct. False is returned if
 * the image could not be read.
 */
static int readpng(void const *filedata, pnginfo *png)
{
    int const sigsize = 8;
    memstreaminfo stream;
    int j;

    if (png_sig_cmp((void*)filedata, 0, sigsize))
        return FALSE;
    png->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png->png)
        return FALSE;
    png->info = png_create_info_struct(png->png);
    if (!png->info)
        return FALSE;
    if (setjmp(png_jmpbuf(png->png)))
        return FALSE;

    stream.data = filedata;
    stream.offset = 0;
    png_set_read_fn(png->png, &stream, readcallback);

    png_read_info(png->png, png->info);
    png->width = png_get_image_width(png->png, png->info);
    png->height = png_get_image_height(png->png, png->info);
    if (png_get_color_type(png->png, png->info) != PNG_COLOR_TYPE_RGBA) {
        warn("internal error: PNGs are not in RGBA format");
        return FALSE;
    }

    png->rows = allocate(png->height * sizeof *png->rows);
    for (j = 0 ; j < png->height ; ++j)
        png->rows[j] = allocate(png_get_rowbytes(png->png, png->info));
    png_read_image(png->png, png->rows);
    return TRUE;
}

/* Deallocate all memory associated with a PNG.
 */
static void freepng(pnginfo *png)
{
    int j;

    for (j = 0 ; j < png->height ; ++j)
        deallocate(png->rows[j]);
    deallocate(png->rows);
    png->rows = NULL;
    png_destroy_read_struct(&png->png, &png->info, NULL);
}

/* Return an RGBA surface.
 */
static SDL_Surface *makesurface(int width, int height)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int const rmask = 0xFF000000, gmask = 0x00FF0000,
              bmask = 0x0000FF00, amask = 0x000000FF;
#else
    int const rmask = 0x000000FF, gmask = 0x0000FF00,
              bmask = 0x00FF0000, amask = 0xFF000000;
#endif

    return SDL_CreateRGBSurface(0, width, height, 32,
                                rmask, gmask, bmask, amask);
}

/* Copy decoded pixel data to an SDL surface.
 */
static void rowstosurface(SDL_Surface *surface, pnginfo const *png)
{
    char *p;
    int j;

    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);
    p = surface->pixels;
    for (j = 0 ; j < png->height ; ++j) {
        memcpy(p, png->rows[j], 4 * png->width);
        p += surface->pitch;
    }
    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
}

/*
 * Internal function.
 */

/* Translate the contents of a PNG file currently in memory over to an
 * SDL object.
 */
SDL_Surface *pngtosurface(void const *pngdata)
{
    SDL_Surface *surface;
    pnginfo image;

    if (!readpng(pngdata, &image))
        return NULL;
    surface = makesurface(image.width, image.height);
    rowstosurface(surface, &image);
    freepng(&image);
    return surface;
}
