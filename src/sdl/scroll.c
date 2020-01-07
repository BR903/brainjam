/* sdl/scroll.c: managing a scrollbar element.
 */

#include "SDL.h"
#include "./gen.h"
#include "internal.h"
#include "scroll.h"

/* Calculate the rectangle of the scroll thumb, using the scrollbar's
 * current value along with its range and page size. For the sake of
 * usability and aesthetics, the thumb's height is constrained to
 * never be shorter than twice its width.
 */
static void getthumb(scrollbar const *scroll, SDL_Rect *rect)
{
    int n;

    rect->x = scroll->pos.x;
    rect->y = scroll->pos.y;
    rect->w = scroll->pos.w;
    if (scroll->range <= 0) {
        rect->h = 0;
        return;
    }
    n = scroll->range + scroll->pagesize;
    rect->h = (scroll->pos.h * scroll->pagesize) / n;
    if (rect->h < 2 * rect->w)
        rect->h = 2 * rect->w;
    rect->y += scroll->value * (scroll->pos.h - rect->h) / scroll->range;
}

/* Given a y-coordinate, calculate the corresponding value scaled to
 * the scrollbar's range.
 */
static int scaleoffset(scrollbar const *scroll, int yoffset)
{
    SDL_Rect rect;

    if (scroll->pos.h <= 0)
        return 0;
    getthumb(scroll, &rect);
    if (rect.h <= 0)
        return 0;
    return (scroll->range * yoffset) / (scroll->pos.h - rect.h);
}

/* Force the scrollbar's value into its range.
 */
static void clampvalue(scrollbar *scroll)
{
    if (scroll->value < 0)
        scroll->value = 0;
    if (scroll->value >= scroll->range)
        scroll->value = scroll->range - 1;
}

/*
 * Internal functions.
 */

/* Render a scrollbar.
 */
void scrollrender(scrollbar const *scroll)
{
    SDL_Rect rect;
    Uint8 r, g, b, a;

    SDL_GetRenderDrawColor(_graph.renderer, &r, &g, &b, &a);
    SDL_SetRenderDrawColor(_graph.renderer, colors(_graph.bkgndcolor),
                           SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(_graph.renderer, &scroll->pos);
    SDL_SetRenderDrawColor(_graph.renderer, colors(_graph.dimmedcolor),
                           SDL_ALPHA_OPAQUE);
    getthumb(scroll, &rect);
    SDL_RenderFillRect(_graph.renderer, &rect);
    SDL_SetRenderDrawColor(_graph.renderer, r, g, b, a);
}

/* Identify and manage any mouse events that involve manipulation of
 * the given scroll bar. Mouse clicks on the scrollbar cause the thumb
 * to jump to that position, while clicks on the thumb itself allow it
 * to be vertically dragged.
 */
int scrolleventhandler(SDL_Event *event, scrollbar *scroll)
{
    static scrollbar *dragging = NULL;
    static int fromy, fromvalue;
    SDL_Rect rect;
    int x, y;

    switch (event->type) {
      case SDL_MOUSEBUTTONDOWN:
        if (dragging == scroll)
            dragging = NULL;
        if (rectcontains(&scroll->pos, event->button.x, event->button.y)) {
            getthumb(scroll, &rect);
            if (rectcontains(&rect, event->button.x, event->button.y)) {
                dragging = scroll;
                fromy = event->button.y;
                fromvalue = scroll->value;
            } else {
                scroll->value = scaleoffset(scroll,
                                            event->button.y - scroll->pos.y);
                clampvalue(scroll);
            }
            return TRUE;
        }
        break;
      case SDL_MOUSEMOTION:
        if (dragging == scroll) {
            scroll->value = fromvalue + 
                                scaleoffset(scroll, event->motion.y - fromy);
            clampvalue(scroll);
            return TRUE;
        }
        break;
      case SDL_MOUSEBUTTONUP:
        if (dragging == scroll) {
            scroll->value = fromvalue +
                                scaleoffset(scroll, event->button.y - fromy);
            clampvalue(scroll);
            dragging = NULL;
            return TRUE;
        }
        break;
      case SDL_MOUSEWHEEL:
        SDL_GetMouseState(&x, &y);
        if (dragging == scroll)
            dragging = NULL;
        if (rectcontains(&scroll->pos, x, y)) {
            scroll->value -= scroll->linesize * event->wheel.y;
            clampvalue(scroll);
            return TRUE;
        }
        break;
    }
    return FALSE;
}
