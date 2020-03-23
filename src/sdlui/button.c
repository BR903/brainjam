/* sdlui/button.c: button creation.
 */

#include <SDL.h>
#include <SDL_ttf.h>
#include "./gen.h"
#include "internal.h"
#include "image.h"
#include "button.h"

/*
 * Since a button can be in a limited set of states at any given time,
 * each state is accompanied by a different image. These images are
 * all stored in a single texture, with the state value used as an
 * offset within it. The button creation functions generate all such
 * images upon initialization. The button-handling code can then
 * simply render the appropriate image for the current state.
 */

/* A line one pixel wide is assumed to be clearly visible.
 */
#define linesize 1

/*
 * Basic rendering functions.
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

/* Draw the outline of a rectangle open on the left-hand side.
 */
static void drawopenleftrect(SDL_Surface *image, Uint32 color,
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
static SDL_Surface *makeroundedsurface(int w, int h, int shadowdepth)
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

/*
 * Pushbutton rendering functions.
 */

/* Create a column of four pushbutton backgrounds with the given
 * dimensions. The first three backgrounds are for pushbuttons that
 * are not pressed, so the background is one with convex shading. The
 * fourth is for a pressed button, and so has a flat background.
 */
static SDL_Surface *makepushbuttonbkgndset(int w, int h)
{
    int const shading = 72;
    SDL_Surface *image, *bkgnd;
    SDL_Rect rect;
    Uint32 color;
    int i;

    image = SDL_CreateRGBSurface(0, w, 4 * h, 32, 0x000000FF, 0x0000FF00,
                                                  0x00FF0000, 0x00000000);
    color = SDL_MapRGB(image->format, colors3(_graph.bkgndcolor));
    SDL_FillRect(image, NULL, color);

    bkgnd = makeroundedsurface(w, h, shading);
    rect.x = 0;
    rect.y = 0;
    rect.w = w;
    rect.h = h;
    color = SDL_MapRGB(image->format, colors3(_graph.defaultcolor));
    for (i = 0 ; i < 4 ; ++i) {
        if (i != (BSTATE_DOWN | BSTATE_HOVER))
            SDL_BlitSurface(bkgnd, NULL, image, &rect);
        drawrect(image, color, rect.x, rect.y, rect.w, rect.h);
        rect.y += h;
    }
    SDL_FreeSurface(bkgnd);

    return image;
}

/* Create a column of eight popup button backgrounds with the given
 * dimensions. The first four backgrounds are the same as for normal
 * pushbutton. The next four are for the selected state, and are all
 * drawn with flat backgrounds and no border on the left-hand side.
 */
static SDL_Surface *makeopenleftbkgndset(int w, int h)
{
    int const shading = 96;
    SDL_Surface *image, *bkgnd;
    SDL_Rect rect;
    Uint32 color;
    int i;

    image = SDL_CreateRGBSurface(0, w, 8 * h, 32, 0x000000FF, 0x0000FF00,
                                                  0x00FF0000, 0x00000000);
    color = SDL_MapRGB(image->format, colors3(_graph.bkgndcolor));
    SDL_FillRect(image, NULL, color);

    bkgnd = makeroundedsurface(w, h, shading);
    rect.x = 0;
    rect.y = 0;
    rect.w = w;
    rect.h = h;
    color = SDL_MapRGB(image->format, colors3(_graph.defaultcolor));
    for (i = 0 ; i < 8 ; ++i) {
        if ((i & BSTATE_SELECT) == 0 && i != (BSTATE_DOWN | BSTATE_HOVER))
            SDL_BlitSurface(bkgnd, NULL, image, &rect);
        if (i & BSTATE_SELECT)
            drawopenleftrect(image, color, &rect);
        else
            drawrect(image, color, rect.x, rect.y, rect.w, rect.h);
        rect.y += h;
    }
    SDL_FreeSurface(bkgnd);

    return image;
}

/* Create a column of pushbutton labels for four different pushbutton
 * states. The four states are: normal (enabled but not in use),
 * hovered (the mouse is over the button but not pressing it),
 * disabled, and pushed. The returned image repeats the graphic
 * vertically, with different coloration appropriate for each state.
 */
static SDL_Surface *makepushbuttonlabelset(SDL_Surface *label, int w, int h)
{
    int const mods[] = { 160, 192, 160, 255 };
    SDL_Surface *image;
    SDL_Rect rect;
    int i;

    image = SDL_CreateRGBSurface(0, w, 4 * h, 32, 0x000000FF, 0x0000FF00,
                                                  0x00FF0000, 0xFF000000);
    rect.x = (w - label->w) / 2;
    rect.y = (h - label->h) / 2;
    rect.w = label->w;
    rect.h = label->h;
    for (i = 0 ; i < 4 ; ++i) {
        SDL_SetSurfaceColorMod(label, mods[i], mods[i], mods[i]);
        SDL_SetSurfaceAlphaMod(label, i == BSTATE_DISABLED ? 128 : 255);
        SDL_BlitSurface(label, NULL, image, &rect);
        rect.y += h;
    }
    return image;
}

/*
 * Checkbox rendering functions.
 */

/* Determine the dimensions and placement of a checkbox, based on the
 * font used to render the accompanying text. In addition to filling
 * in the rectangle coordinates, the function's return value gives the
 * horizontal offset at which the text should be placed.
 */
static int placecheckbox(TTF_Font *font, SDL_Rect *box)
{
    int baseline, boxsize, margin;

    margin = TTF_FontLineSkip(font);
    baseline = TTF_FontAscent(font);
    TTF_GlyphMetrics(font, 't', NULL, NULL, NULL, &boxsize, NULL);
    if (boxsize < 10 * linesize)
        boxsize = 10 * linesize;
    if (margin < boxsize + 2 * linesize)
        margin = boxsize + 2 * linesize;

    box->x = linesize;
    box->y = baseline - boxsize;
    box->w = boxsize;
    box->h = boxsize;

    return margin;
}

/* Create the basic image of a checkbox button, with text displayed to
 * the right of a square. The returned image has a transparent
 * background. In addition to supplying an image, the function fills
 * in the coordinates of two rectangles. The first is the rectangle
 * for filling the square with a mark (when the button is selected).
 * The second is a rectangle around the inside of the square, to show
 * a highlight (when the button is down or hovered).
 */
static SDL_Surface *makecheckboxbaseimage(char const *str,
                                          SDL_Rect *mark, SDL_Rect *boxinner)
{
    SDL_Surface *label, *image;
    SDL_Rect rect;
    Uint32 color;
    int offset;

    offset = placecheckbox(_graph.smallfont, &rect);
    boxinner->x = rect.x + 1 * linesize;
    boxinner->y = rect.y + 1 * linesize;
    boxinner->w = rect.w - 2 * linesize;
    boxinner->h = rect.h - 2 * linesize;
    mark->x = rect.x + 3 * linesize;
    mark->y = rect.y + 3 * linesize;
    mark->w = rect.w - 6 * linesize;
    mark->h = rect.h - 6 * linesize;

    label = TTF_RenderUTF8_Blended(_graph.smallfont, str, _graph.defaultcolor);
    image = SDL_CreateRGBSurface(0, offset + label->w, label->h, 32,
                                 0x000000FF, 0x0000FF00,
                                 0x00FF0000, 0xFF000000);

    color = SDL_MapRGBA(image->format, colors4(_graph.defaultcolor));
    drawrect(image, color, rect.x, rect.y, rect.w, rect.h);

    rect.x = offset;
    rect.y = 0;
    rect.w = label->w;
    rect.h = label->h;
    SDL_BlitSurface(label, NULL, image, &rect);
    SDL_FreeSurface(label);

    return image;
}

/* Create a columnn of eight checkbox images, one for each possible
 * button state. The first four are the various active/enable states
 * with the checkbox empty, and then the second half are the same set
 * of states with the checkbox marked.
 */
static SDL_Surface *makecheckboxset(char const *text)
{
    SDL_Surface *graphic, *image;
    SDL_Rect rect, mark, highlight;
    Uint32 textval, dimval;
    int i, j;

    graphic = makecheckboxbaseimage(text, &mark, &highlight);

    textval = SDL_MapRGBA(graphic->format, colors4(_graph.defaultcolor));
    dimval = SDL_MapRGBA(graphic->format, colors4(_graph.dimmedcolor));

    image = SDL_CreateRGBSurface(0, graphic->w, 8 * graphic->h, 32,
                                 0x000000FF, 0x0000FF00,
                                 0x00FF0000, 0x00000000);
    SDL_FillRect(image, NULL,
                 SDL_MapRGB(image->format, colors3(_graph.bkgndcolor)));

    rect.x = 0;
    rect.y = 0;
    rect.w = graphic->w;
    rect.h = graphic->h;
    for (j = 0 ; j < 2 ; ++j) {
        for (i = 0 ; i < 4 ; ++i) {
            SDL_SetSurfaceAlphaMod(graphic, i == BSTATE_DISABLED ? 128 : 255);
            SDL_BlitSurface(graphic, NULL, image, &rect);
            if (i & BSTATE_HOVER)
                drawrect(image, i & BSTATE_DOWN ? textval : dimval,
                         highlight.x, highlight.y, highlight.w, highlight.h);
            rect.y += rect.h;
            highlight.y += rect.h;
        }
        SDL_FillRect(graphic, &mark, textval);
    }

    SDL_FreeSurface(graphic);
    return image;
}

/*
 * Internal functions.
 */

/* Initialize a pushbutton labeled with an image. Images for the
 * different states of the button are rendered onto a texture. For a
 * simple pushbutton there is no selection state, so the basic images
 * are simply used twice.
 */
void makeimagebutton(button *push, int iconid)
{
    SDL_Surface *graphic, *image, *labels;
    int i;

    graphic = getbuttonlabel(iconid);
    push->pos.x = 0;
    push->pos.y = 0;
    push->pos.w = graphic->w + _graph.margin;
    push->pos.h = graphic->h + _graph.margin;

    image = makepushbuttonbkgndset(push->pos.w, push->pos.h);
    labels = makepushbuttonlabelset(graphic, push->pos.w, push->pos.h);
    SDL_BlitSurface(labels, NULL, image, NULL);
    push->texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(graphic);
    SDL_FreeSurface(labels);
    SDL_FreeSurface(image);

    push->state = BSTATE_NORMAL;
    push->visible = 1;
    push->action = 0;
    push->rects = allocate(8 * sizeof *push->rects);
    for (i = 0 ; i < 8 ; ++i) {
        push->rects[i].x = 0;
        push->rects[i].y = (i % 4) * push->pos.h;
        push->rects[i].w = push->pos.w;
        push->rects[i].h = push->pos.h;
    }
}

/* Initialize a pushbutton that controls the display of a popup
 * "window", adjacent to the button's left-hand side. The popup window
 * is displayed when the button is selected, and hidden otherwise. The
 * button object does not draw the popup itself, but simply leaves an
 * opening on the left edge of the button for the popup to merge with.
 */
void makepopupbutton(button *popup, int iconid)
{
    SDL_Surface *graphic, *image, *labels;
    SDL_Rect rect;
    int i;

    graphic = getbuttonlabel(iconid);
    popup->pos.x = 0;
    popup->pos.y = 0;
    popup->pos.w = graphic->w + _graph.margin;
    popup->pos.h = graphic->h + _graph.margin;

    image = makeopenleftbkgndset(popup->pos.w, popup->pos.h);
    labels = makepushbuttonlabelset(graphic, popup->pos.w, popup->pos.h);
    rect.x = 0;
    rect.y = 0;
    rect.w = labels->w;
    rect.h = labels->h;
    SDL_BlitSurface(labels, NULL, image, &rect);
    rect.y += rect.h;
    SDL_BlitSurface(labels, NULL, image, &rect);
    popup->texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(graphic);
    SDL_FreeSurface(labels);
    SDL_FreeSurface(image);

    popup->state = BSTATE_NORMAL;
    popup->visible = 1;
    popup->action = 0;
    popup->rects = allocate(8 * sizeof *popup->rects);
    for (i = 0 ; i < 8 ; ++i) {
        popup->rects[i].x = 0;
        popup->rects[i].y = i * popup->pos.h;
        popup->rects[i].w = popup->pos.w;
        popup->rects[i].h = popup->pos.h;
    }
}

/* Create a checkbox button, with the checkbox square drawn to the
 * left of the text. All of the checkbox's states are rendered into a
 * single texture.
 */
void makecheckbox(button *chkbox, char const *str)
{
    SDL_Surface *image;
    int i;

    image = makecheckboxset(str);

    chkbox->pos.x = 0;
    chkbox->pos.y = 0;
    chkbox->pos.w = image->w;
    chkbox->pos.h = image->h / 8;

    chkbox->texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(image);

    chkbox->state = BSTATE_NORMAL;
    chkbox->visible = 1;
    chkbox->action = 0;
    chkbox->rects = allocate(8 * sizeof *chkbox->rects);
    for (i = 0 ; i < 8 ; ++i) {
        chkbox->rects[i].x = 0;
        chkbox->rects[i].y = i * chkbox->pos.h;
        chkbox->rects[i].w = chkbox->pos.w;
        chkbox->rects[i].h = chkbox->pos.h;
    }
}
