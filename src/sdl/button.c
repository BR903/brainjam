/* sdl/button.c: button creation.
 */

#include <SDL.h>
#include <SDL_ttf.h>
#include "./gen.h"
#include "internal.h"
#include "images.h"
#include "button.h"

/*
 * Since a button can be in a limited set of states at any given time,
 * each state is accompanied by a separate image. These images are all
 * stored in a single texture, with the state value used as an offset
 * within it. The button creation functions therefore generate each
 * image during initialization and return the generated texture. The
 * button-handling code can then simply blit the appropriate image for
 * the current state.
 */

/* A line one pixel wide is assumed to be visible. Change this if
 * lines need to be thicker to be comfortably clear.
 */
#define linesize 1

/*
 * Rendering functions.
 */

/* Draw the outline of a rectangle in a solid color on a surface.
 */
static void drawrect(SDL_Surface *image, Uint32 color,
                     int x, int y, int w, int h)
{
    SDL_Rect rect;

    rect.x = x;
    rect.y = y;
    rect.w = linesize;
    rect.h = h;
    SDL_FillRect(image, &rect, color);
    rect.x += w - linesize;
    SDL_FillRect(image, &rect, color);
    rect.x = x;
    rect.w = w;
    rect.h = linesize;
    SDL_FillRect(image, &rect, color);
    rect.y += h - linesize;
    SDL_FillRect(image, &rect, color);
}

/* Draw the outline of a rectangle, minus the left-hand side.
 */
static void drawopenrect(SDL_Surface *image, Uint32 color,
                         SDL_Rect const *frame)
{
    SDL_Rect rect;

    rect.x = frame->x;
    rect.y = frame->y;
    rect.w = frame->w;
    rect.h = linesize;
    SDL_FillRect(image, &rect, color);
    rect.y += frame->h - linesize;
    SDL_FillRect(image, &rect, color);
    rect.x += frame->w - linesize;
    rect.y = frame->y;
    rect.w = linesize;
    rect.h = frame->h;
    SDL_FillRect(image, &rect, color);
}

/* Return a surface with the given dimensions and filled with a gray
 * gradient suggesting a convex surface. shadowdepth indicates how
 * much of a grayscale difference to move through.
 */
static SDL_Surface *makeshadedsurface(int w, int h, int shadowdepth)
{
    SDL_Color shading[256];
    SDL_Surface *surface;
    Uint8 *p;
    int offset, i;

    for (i = 0 ; i < 256 ; ++i)
        shading[i].r = shading[i].g = shading[i].b = 255 - i;
    surface = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);
    SDL_SetPaletteColors(surface->format->palette, shading, 0, 256);

    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);
    offset = h / 4;
    for (i = 0, p = surface->pixels ; i < h ; ++i, p += surface->pitch)
        memset(p, (abs(i - offset) * shadowdepth) / (h - offset), w);
    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
    drawrect(surface, 255, 0, 0, w, h);

    return surface;
}

/* Initialize a pushbutton with the given image as its face. Graphics
 * for all of the different states of the button are rendered onto a
 * single texture.
 */
static void makegraphicbutton(button *pushbutton, SDL_Surface *graphic)
{
    int const mods[] = { 160, 192, 160, 255 };
    SDL_Surface *image, *bkgnd;
    SDL_Rect rect;
    Uint32 color;
    int i;

    pushbutton->pos.x = 0;
    pushbutton->pos.y = 0;
    pushbutton->pos.w = graphic->w + _graph.margin;
    pushbutton->pos.h = graphic->h + _graph.margin;
    image = SDL_CreateRGBSurface(0,
                                 pushbutton->pos.w, 4 * pushbutton->pos.h, 32,
                                 0x000000FF, 0x0000FF00,
                                 0x00FF0000, 0x00000000);
    bkgnd = makeshadedsurface(pushbutton->pos.w, pushbutton->pos.h, 72);
    rect.x = 0;
    rect.y = 0;
    rect.w = pushbutton->pos.w;
    rect.h = pushbutton->pos.h;
    SDL_BlitSurface(bkgnd, NULL, image, &rect);
    rect.y += pushbutton->pos.h;
    SDL_BlitSurface(bkgnd, NULL, image, &rect);
    rect.y += pushbutton->pos.h;
    SDL_BlitSurface(bkgnd, NULL, image, &rect);
    rect.y += pushbutton->pos.h;
    SDL_FreeSurface(bkgnd);
    color = SDL_MapRGB(image->format, colors3(_graph.bkgndcolor));
    SDL_FillRect(image, &rect, color);
    color = SDL_MapRGB(image->format, colors3(_graph.defaultcolor));
    drawrect(image, color, rect.x, rect.y, rect.w, rect.h);

    rect.x = (pushbutton->pos.w - graphic->w) / 2;
    rect.y = (pushbutton->pos.h - graphic->h) / 2;
    rect.w = graphic->w;
    rect.h = graphic->h;
    for (i = 0 ; i < 4 ; ++i) {
        SDL_SetSurfaceColorMod(graphic, mods[i], mods[i], mods[i]);
        SDL_SetSurfaceAlphaMod(graphic,
                               i == BSTATE_DISABLED ? 128 : SDL_ALPHA_OPAQUE);
        SDL_BlitSurface(graphic, NULL, image, &rect);
        rect.y += pushbutton->pos.h;
    }

    pushbutton->texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(image);
    pushbutton->state = BSTATE_NORMAL;
    pushbutton->visible = 1;
    pushbutton->action = 0;
    pushbutton->rects = allocate(8 * sizeof(SDL_Rect));
    for (i = 0 ; i < 8 ; ++i) {
        pushbutton->rects[i].x = 0;
        pushbutton->rects[i].y = (i % 4) * pushbutton->pos.h;
        pushbutton->rects[i].w = pushbutton->pos.w;
        pushbutton->rects[i].h = pushbutton->pos.h;
    }
}

/*
 * Internal functions.
 */

/* Initialize a pushbutton with an icon image.
 */
void makeimagebutton(button *pushbutton, int iconid)
{
    SDL_Surface *image;

    image = getbuttonicon(iconid);
    makegraphicbutton(pushbutton, image);
    SDL_FreeSurface(image);
}

/* Create a checkbox button, with the checkbox square drawn to the
 * left of the text. All of the checkbox's states are rendered into a
 * single texture.
 */
void makecheckbox(button *chkbox, char const *str)
{
    SDL_Surface *textimage, *image;
    SDL_Rect srcrect, destrect;
    Uint32 textval, bkgndval, dimtextval;
    int baseline, boxsize, i;

    baseline = TTF_FontAscent(_graph.smallfont);
    TTF_GlyphMetrics(_graph.smallfont, 't', NULL, NULL, NULL, &boxsize, NULL);
    if (boxsize < 10 * linesize)
        boxsize = 10 * linesize;
    textimage = TTF_RenderUTF8_Blended(_graph.smallfont, str,
                                       _graph.defaultcolor);
    chkbox->pos.x = 0;
    chkbox->pos.y = 0;
    chkbox->pos.w = textimage->w + textimage->h;
    chkbox->pos.h = textimage->h;

    image = SDL_CreateRGBSurface(0, chkbox->pos.w, 8 * chkbox->pos.h,
                                 textimage->format->BitsPerPixel,
                                 textimage->format->Rmask,
                                 textimage->format->Gmask,
                                 textimage->format->Bmask,
                                 textimage->format->Amask);
    bkgndval = SDL_MapRGB(image->format, colors3(_graph.bkgndcolor));
    textval = SDL_MapRGB(image->format, colors3(_graph.defaultcolor));
    dimtextval = SDL_MapRGB(image->format, colors3(_graph.dimmedcolor));
    SDL_FillRect(image, NULL, bkgndval);
    destrect.x = image->w - textimage->w;
    destrect.y = 0;
    SDL_BlitSurface(textimage, NULL, image, &destrect);
    SDL_FreeSurface(textimage);
    drawrect(image, textval, linesize, baseline - boxsize, boxsize, boxsize);
    srcrect.x = 0;
    srcrect.y = 0;
    srcrect.w = chkbox->pos.w;
    srcrect.h = chkbox->pos.h;
    destrect = srcrect;
    destrect.y += destrect.h;
    SDL_BlitSurface(image, &srcrect, image, &destrect);
    drawrect(image, dimtextval,
             2 * linesize, destrect.y + baseline - boxsize + linesize,
             boxsize - 2 * linesize, boxsize - 2 * linesize);
    destrect.y += 2 * destrect.h;
    SDL_BlitSurface(image, &srcrect, image, &destrect);
    drawrect(image, textval,
             2 * linesize, destrect.y + baseline - boxsize + linesize,
             boxsize - 2 * linesize, boxsize - 2 * linesize);
    srcrect.h *= 4;
    destrect.y = srcrect.h;
    SDL_BlitSurface(image, &srcrect, image, &destrect);
    destrect.x = 4 * linesize;
    destrect.y += baseline - boxsize + 3 * linesize;
    destrect.w = boxsize - 6 * linesize;
    destrect.h = boxsize - 6 * linesize;
    SDL_FillRect(image, &destrect, textval);
    destrect.y += chkbox->pos.h;
    SDL_FillRect(image, &destrect, textval);
    destrect.y += 2 * chkbox->pos.h;
    SDL_FillRect(image, &destrect, textval);

    SDL_SetSurfaceBlendMode(image, SDL_BLENDMODE_BLEND);
    SDL_SetSurfaceAlphaMod(image, 128);
    srcrect.x = 0;
    srcrect.y = 0;
    srcrect.w = chkbox->pos.w;
    srcrect.h = chkbox->pos.h;
    destrect.x = 0;
    destrect.y = 2 * chkbox->pos.h;
    destrect.w = chkbox->pos.w;
    destrect.h = chkbox->pos.h;
    SDL_BlitSurface(image, &srcrect, image, &destrect);
    srcrect.y += 4 * chkbox->pos.h;
    destrect.y += 4 * chkbox->pos.h;
    SDL_BlitSurface(image, &srcrect, image, &destrect);
    SDL_SetSurfaceAlphaMod(image, SDL_ALPHA_OPAQUE);
    SDL_SetSurfaceBlendMode(image, SDL_BLENDMODE_NONE);

    chkbox->texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(image);
    chkbox->state = 0;
    chkbox->visible = 0;
    chkbox->action = 0;
    chkbox->rects = allocate(8 * sizeof(SDL_Rect));
    for (i = 0 ; i < 8 ; ++i) {
        chkbox->rects[i].x = 0;
        chkbox->rects[i].y = i * chkbox->pos.h;
        chkbox->rects[i].w = chkbox->pos.w;
        chkbox->rects[i].h = chkbox->pos.h;
    }
}

/* Create a pushbutton that controls the display of a popup "window",
 * adjacent to the button's left-hand side. The button object does not
 * draw the popup itself, but simply leaves an opening on the left
 * edge of the button for the popup to merge with.
 */
void makepopupbutton(button *popup, int iconid)
{
    int const mods[] = { 160, 192, 160, 255 };
    SDL_Surface *graphic, *image, *bkgnd;
    SDL_Rect rect;
    Uint32 bkgndval, textval;
    int i;

    graphic = getbuttonicon(iconid);
    popup->pos.x = 0;
    popup->pos.y = 0;
    popup->pos.w = graphic->w + _graph.margin;
    popup->pos.h = graphic->h + _graph.margin;
    image = SDL_CreateRGBSurface(0, popup->pos.w, 8 * popup->pos.h, 32,
                                 0x000000FF, 0x0000FF00,
                                 0x00FF0000, 0x00000000);
    bkgnd = makeshadedsurface(popup->pos.w, popup->pos.h, 96);
    rect.x = 0;
    rect.y = 0;
    rect.w = popup->pos.w;
    rect.h = popup->pos.h;
    SDL_BlitSurface(bkgnd, NULL, image, &rect);
    rect.y += popup->pos.h;
    SDL_BlitSurface(bkgnd, NULL, image, &rect);
    rect.y += popup->pos.h;
    SDL_BlitSurface(bkgnd, NULL, image, &rect);
    rect.y += popup->pos.h;
    SDL_FreeSurface(bkgnd);
    bkgndval = SDL_MapRGB(image->format, colors3(_graph.bkgndcolor));
    textval = SDL_MapRGB(image->format, colors3(_graph.defaultcolor));
    SDL_FillRect(image, &rect, bkgndval);
    drawrect(image, textval, rect.x, rect.y, rect.w, rect.h);
    for (i = 0 ; i < 4 ; ++i) {
        rect.y += popup->pos.h;
        SDL_FillRect(image, &rect, bkgndval);
        drawopenrect(image, textval, &rect);
    }

    rect.x = (popup->pos.w - graphic->w) / 2;
    rect.y = (popup->pos.h - graphic->h) / 2;
    rect.w = graphic->w;
    rect.h = graphic->h;
    for (i = 0 ; i < 8 ; ++i) {
        SDL_SetSurfaceColorMod(graphic, mods[i % 4], mods[i % 4], mods[i % 4]);
        SDL_SetSurfaceAlphaMod(graphic,
                            i % 4 == BSTATE_DISABLED ? 128 : SDL_ALPHA_OPAQUE);
        SDL_BlitSurface(graphic, NULL, image, &rect);
        rect.y += popup->pos.h;
    }
    SDL_FreeSurface(graphic);

    popup->texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(image);
    popup->state = BSTATE_NORMAL;
    popup->visible = 1;
    popup->action = 0;
    popup->rects = allocate(8 * sizeof(SDL_Rect));
    for (i = 0 ; i < 8 ; ++i) {
        popup->rects[i].x = 0;
        popup->rects[i].y = i * popup->pos.h;
        popup->rects[i].w = popup->pos.w;
        popup->rects[i].h = popup->pos.h;
    }
}
