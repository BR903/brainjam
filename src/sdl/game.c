/* game.c: the main game display.
 */

#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include "./gen.h"
#include "./types.h"
#include "./decls.h"
#include "./commands.h"
#include "./decks.h"
#include "./settings.h"
#include "game/game.h"
#include "redo/redo.h"
#include "internal.h"
#include "images.h"
#include "button.h"

/* Macro to encapsulate the process of changing bit values.
 */
#define setbitflag(v, m, f) ((v) = ((f) ? (v) | (m) : (v) & ~(m)))

/* The data describing a temporary graphical object drawn on top of
 * the basic game display.
 */
typedef struct overlayinfo {
    int         inuse;          /* true if this struct is being used */
    int         card;           /* object is a playing card */
    int         icon;           /* object is an icon */
    SDL_Texture *texture;       /* object is an arbitrary texture */
    SDL_Rect    pos;            /* location of object on display */
    int         alpha;          /* opacity of object on display */
} overlayinfo;

/* The data needed to free an overlay object in an animation callback,
 * before chaining to the real callback function.
 */
typedef struct animcleanupparams {
    overlayinfo *overlay;       /* overlay to be freed */
    void       *data;           /* argument to pass to the callback */
    void      (*callback)(void*); /* callback to invoke */
} animcleanupparams;

/* A neutral yet not-gray background for the game display.
 */
static SDL_Color const tablecolor = { 175, 191, 239, 255 };

/* Pushbuttons that are found in the game display.
 */
static button helpbutton, backbutton, optionsbutton;

/* Checkboxes appearing in the options popup dialog.
 */
static button keyschkbox, animchkbox, autochkbox, redochkbox;

/* The locations of the game display elements.
 */
static SDL_Rect foundations;            /* location of the four foundations */
static SDL_Rect reserves;               /* location of the four reserves */
static SDL_Rect tableau;                /* location of the tableau stacks */
static SDL_Rect sidebar;                /* location of the sidebar info */
static SDL_Rect saveiconpos;            /* location of the write indicator */
static SDL_Rect dialog;                 /* location of the options popup */
static SDL_Point dialogoutline[6];      /* outline around the options popup */
static SDL_Point status;                /* position of the stuck/done alerts */
static SDL_Point bookmark;              /* position of the bookmark alert */
static SDL_Point movecount;             /* position of the move count */
static SDL_Point bettercount;           /* position of the better indicator */
static SDL_Point bestcount;             /* position of the best move count */
static SDL_Point bestknowncount;        /* position of the lowest count */

/* An array holding the location of each place in the layout.
 */
static SDL_Rect placelocs[NPLACES];

/* Image of a dot (made via the bullet character), and its size.
 */
static SDL_Texture *dot;
static SDL_Point dotsize;

/* Image of an equal sign, and its size.
 */
static SDL_Texture *equal;
static SDL_Point equalsize;

/* Images of keyboard keys for the each place, and the size of one key.
 */
static SDL_Texture *keyguidetexture;
static int keyguidesize;

/* The visible state of the game. Because the display can require
 * redisplay due to events not initiated by the calling code, the
 * program needs to stash these values each time rendergame() is
 * invoked.
 */
static gameplayinfo const *gameplay;    /* current game state */
static redo_position const *position;   /* current redo position */
static int bookmarkflag;                /* true if a bookmark exists */

/* Size of the user's most recent best solution for the current game.
 * A negative value indicates that this variable has not yet been
 * initialized. A zero value indicates that the user has yet to create
 * a solution.
 */
static int prevbestsolutionsize = -1;

/* A pool of temporary animation objects.
 */
static overlayinfo overlays[16];

/* True if card motions should be animated.
 */
static int cardanimation = TRUE;

/* True if the options popup dialog is currently visible.
 */
static int optionsopen = FALSE;

/*
 * Determining the layout.
 */

/* Calculate the locations and positions of the various elements that
 * make up the game display: The eight single-card places, the eight
 * stacks of the tableau, the alert images, the various counters in
 * the right-hand column, the options popup dialog and its checkboxes,
 * and the back button and help button. This function also fills in
 * the values for the placelocs array.
 */
static SDL_Point setlayout(SDL_Point display)
{
    SDL_Point size;
    SDL_Point spacing;
    int lineheight, largelineheight;
    int i;

    spacing.x = _graph.cardsize.x / 8;
    spacing.y = _graph.cardsize.y / 16;
    lineheight = TTF_FontLineSkip(_graph.smallfont);

    foundations.x = _graph.margin;
    foundations.y = _graph.margin;
    foundations.w = 4 * _graph.cardsize.x + 3 * spacing.x;
    foundations.h = _graph.cardsize.y;

    reserves.x = foundations.x + foundations.w + 4 * spacing.x;
    reserves.y = foundations.y;
    reserves.w = foundations.w;
    reserves.h = foundations.h + lineheight;

    status.x = reserves.x - 2 * spacing.x - getimagewidth(IMAGE_DONE) / 2;
    status.y = reserves.y + (reserves.h - getimageheight(IMAGE_DONE)) / 2;

    tableau.x = _graph.margin + spacing.x;
    tableau.y = reserves.y + reserves.h + 2 * spacing.y;
    tableau.w = 8 * _graph.cardsize.x + 7 * spacing.x;
    tableau.h = display.y - tableau.y;

    if (reserves.x + reserves.w > tableau.x + tableau.w)
        sidebar.x = reserves.x + reserves.w + _graph.margin;
    else
        sidebar.x = tableau.x + tableau.w + _graph.margin;
    sidebar.y = _graph.margin;
    TTF_SizeUTF8(_graph.largefont, "8888", &sidebar.w, &largelineheight);
    if (sidebar.w < optionsbutton.pos.w)
        sidebar.w = optionsbutton.pos.w;
    if (sidebar.w < helpbutton.pos.w)
        sidebar.w = helpbutton.pos.w;
    i = getimagewidth(IMAGE_DONE);
    if (sidebar.w < i)
        sidebar.w = i;
    i = getimagewidth(IMAGE_BOOKMARK);
    if (sidebar.w < i)
        sidebar.w = i;
    sidebar.w += _graph.margin;
    sidebar.h = display.y - helpbutton.pos.h - backbutton.pos.h;
    sidebar.h -= 4 * _graph.margin;

    movecount.x = sidebar.x + sidebar.w;
    movecount.y = sidebar.y;
    bettercount.x = movecount.x;
    bettercount.y = movecount.y + largelineheight;

    bookmark.x = sidebar.x + (sidebar.w - getimagewidth(IMAGE_DONE)) / 2;
    bookmark.y = bettercount.y + lineheight + 4 * spacing.y;

    optionsbutton.pos.x = sidebar.x;
    optionsbutton.pos.y = bookmark.y + getimageheight(IMAGE_BOOKMARK) +
                          3 * _graph.margin;

    bestknowncount.x = movecount.x;
    bestknowncount.y = sidebar.y + sidebar.h - lineheight;

    bestcount.x = movecount.x;
    bestcount.y = bestknowncount.y - largelineheight;

    saveiconpos.w = getimagewidth(IMAGE_WRITE);
    saveiconpos.h = getimageheight(IMAGE_WRITE);
    saveiconpos.x = sidebar.x;
    saveiconpos.y = bestknowncount.y - saveiconpos.h;

    dialog.w = keyschkbox.pos.w;
    if (dialog.w < animchkbox.pos.w)
        dialog.w = animchkbox.pos.w;
    if (dialog.w < autochkbox.pos.w)
        dialog.w = autochkbox.pos.w;
    if (dialog.w < redochkbox.pos.w)
        dialog.w = redochkbox.pos.w;
    dialog.w += 6 * _graph.margin;
    dialog.h = keyschkbox.pos.h + animchkbox.pos.h +
               autochkbox.pos.h + redochkbox.pos.h + 5 * lineheight;
    dialog.x = optionsbutton.pos.x - dialog.w;
    dialog.y = optionsbutton.pos.y + (optionsbutton.pos.h - dialog.h) / 2;

    dialogoutline[0].x = dialog.x + dialog.w;
    dialogoutline[0].y = optionsbutton.pos.y;
    dialogoutline[1].x = dialogoutline[0].x;
    dialogoutline[1].y = dialog.y;
    dialogoutline[2].x = dialog.x;
    dialogoutline[2].y = dialogoutline[1].y;
    dialogoutline[3].x = dialogoutline[2].x;
    dialogoutline[3].y = dialog.y + dialog.h;
    dialogoutline[4].x = dialog.x + dialog.w;
    dialogoutline[4].y = dialogoutline[3].y;
    dialogoutline[5].x = dialogoutline[4].x;
    dialogoutline[5].y = optionsbutton.pos.y + optionsbutton.pos.h;

    keyschkbox.pos.x = dialog.x + 2 * _graph.margin;
    keyschkbox.pos.y = dialog.y + lineheight;
    animchkbox.pos.x = keyschkbox.pos.x;
    animchkbox.pos.y = keyschkbox.pos.y + keyschkbox.pos.h + lineheight;
    autochkbox.pos.x = animchkbox.pos.x;
    autochkbox.pos.y = animchkbox.pos.y + animchkbox.pos.h + lineheight;
    redochkbox.pos.x = autochkbox.pos.x;
    redochkbox.pos.y = autochkbox.pos.y + autochkbox.pos.h + lineheight;

    backbutton.pos.x = display.x - backbutton.pos.w - _graph.margin;
    backbutton.pos.y = display.y - backbutton.pos.h - _graph.margin;
    helpbutton.pos.x = backbutton.pos.x;
    helpbutton.pos.y = backbutton.pos.y - helpbutton.pos.h - _graph.margin;

    for (i = 0 ; i < TABLEAU_PLACE_COUNT ; ++i) {
        placelocs[tableauplace(i)].x = tableau.x +
                                        i * (_graph.cardsize.x + spacing.x);
        placelocs[tableauplace(i)].y = tableau.y;
    }
    for (i = 0 ; i < RESERVE_PLACE_COUNT ; ++i) {
        placelocs[reserveplace(i)].x = reserves.x +
                                        i * (_graph.cardsize.x + spacing.x);
        placelocs[reserveplace(i)].y = reserves.y;
    }
    for (i = 0 ; i < FOUNDATION_PLACE_COUNT ; ++i) {
        placelocs[foundationplace(i)].x = foundations.x +
                                        i * (_graph.cardsize.x + spacing.x);
        placelocs[foundationplace(i)].y = foundations.y;
    }
    for (i = 0 ; i < NPLACES ; ++i) {
        placelocs[i].w = _graph.cardsize.x;
        placelocs[i].h = _graph.cardsize.y;
    }

    size.x = sidebar.x + sidebar.w + spacing.x;
    size.y = tableau.y + _graph.cardsize.y + 2 * spacing.y +
                                             7 * _graph.dropheight;
    return size;
}

/*
 * Managing overlays.
 */

/* Return a currently-unused overlay.
 */
static overlayinfo *getoverlayinfo(void)
{
    static int lastindex = 0;
    int i;

    i = lastindex;
    for (;;) {
        i = (i + 1) % (sizeof overlays / sizeof *overlays);
        if (!overlays[i].inuse) {
            lastindex = i;
            overlays[i].inuse = TRUE;
            return overlays + i;
        }
        if (i == lastindex) {
            warn("internal error: too many overlays!");
            return NULL;
        }
    }
}

/* Put an overlay back into the pool of available overlays. Free the
 * associated overlay texture, if applicable.
 */
static void freeoverlay(overlayinfo *overlay)
{
    if (overlay->texture)
        SDL_DestroyTexture(overlay->texture);
    overlay->texture = NULL;
    overlay->card = 0;
    overlay->icon = 0;
    overlay->inuse = FALSE;
}

/* Callback invoked when an overlay animation completes. The real
 * callback is invoked, and then the overlay is freed.
 */
static void animcleanup(void *data, int done)
{
    animcleanupparams *params;

    (void)done;
    params = data;
    if (params->callback)
        (*params->callback)(params->data);
    if (params->overlay)
        freeoverlay(params->overlay);
    deallocate(params);
}

/* Set the visibility of the contents of the options dialog.
 */
static void setoptionsdisplay(int visible)
{
    keyschkbox.visible = visible;
    animchkbox.visible = visible;
    autochkbox.visible = visible;
    redochkbox.visible = visible;
    optionsopen = visible;
    setbitflag(optionsbutton.state, BSTATE_SELECT, visible);
}

/*
 * Game display animations.
 */

/* Generate an animation to show a number at the location of the
 * user's best solution size, and then have the number slowly fade out
 * as it rises.
 */
static void createreplacementanimation(int value)
{
    animinfo *anim;
    overlayinfo *overlay;
    animcleanupparams *params;
    SDL_Surface *image;
    char buf[8];

    sprintf(buf, "%d", value);
    image = TTF_RenderUTF8_Blended(_graph.largefont, buf, _graph.defaultcolor);
    overlay = getoverlayinfo();
    overlay->icon = 0;
    overlay->card = 0;
    overlay->texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    overlay->pos.x = bestcount.x - image->w;
    overlay->pos.y = bestcount.y;
    overlay->pos.w = image->w;
    overlay->pos.h = image->h;
    overlay->alpha = SDL_ALPHA_OPAQUE;
    SDL_FreeSurface(image);

    params = allocate(sizeof *params);
    params->overlay = overlay;
    params->callback = NULL;

    anim = getaniminfo();
    anim->steps = overlay->pos.h;
    anim->pval1 = &overlay->pos.y;
    anim->destval1 = overlay->pos.y - overlay->pos.h;
    anim->pval2 = &overlay->alpha;
    anim->destval2 = SDL_ALPHA_TRANSPARENT;
    anim->callback = animcleanup;
    anim->data = params;
    startanimation(anim, 30);
}

/* Create an overlay to display the save icon, along with an animation
 * to have it fade out gradually over 2.5 seconds.
 */
static void createsaveanimation(void)
{
    animinfo *anim;
    overlayinfo *overlay;
    animcleanupparams *params;

    overlay = getoverlayinfo();
    overlay->icon = IMAGE_WRITE;
    overlay->card = 0;
    overlay->texture = NULL;
    overlay->pos = saveiconpos;
    overlay->alpha = SDL_ALPHA_OPAQUE;

    params = allocate(sizeof *params);
    params->overlay = overlay;
    params->callback = NULL;

    anim = getaniminfo();
    anim->steps = 255;
    anim->pval1 = &overlay->alpha;
    anim->destval1 = SDL_ALPHA_TRANSPARENT;
    anim->pval2 = NULL;
    anim->callback = animcleanup;
    anim->data = params;
    startanimation(anim, 10);
}

/* Animate a card moving across the layout, with from and to
 * specifying the starting and ending points. An overlay is created to
 * render the card at the starting position, and an animation is used
 * to move the overlay to its ending position. A callback can be
 * attached to the animation, to be invoked once the card has arrived
 * at its destination.
 */
static int createcardanimation(gameplayinfo const *gameplay,
                               card_t card, place_t from, place_t to,
                               void (*callback)(void*), void *data)
{
    overlayinfo *overlay;
    animinfo *anim;
    animcleanupparams *params;
    SDL_Rect rect;

    rect = placelocs[from];
    if (istableauplace(from))
        rect.y += gameplay->depth[from] * _graph.dropheight;

    overlay = getoverlayinfo();
    overlay->icon = 0;
    overlay->card = card;
    overlay->texture = NULL;
    overlay->pos = rect;

    params = allocate(sizeof *params);
    params->overlay = overlay;
    params->callback = callback;
    params->data = data;

    anim = getaniminfo();
    anim->steps = 16;
    anim->pval1 = &overlay->pos.x;
    anim->pval2 = &overlay->pos.y;
    anim->destval1 = placelocs[to].x;
    anim->destval2 = placelocs[to].y;
    if (istableauplace(to))
        anim->destval2 += gameplay->depth[to] * _graph.dropheight;
    anim->callback = animcleanup;
    anim->data = params;

    startanimation(anim, 10);
    return TRUE;
}

/*
 * Rendering the game display.
 */

/* Return true if there are no empty slots in the current layout,
 * indicating that it would be helpful to mark which cards have a
 * legal move available.
 */
static int shouldmarkmoveable(gameplayinfo const *gameplay)
{
    int i;

    for (i = MOVEABLE_PLACE_1ST ; i < MOVEABLE_PLACE_END ; ++i)
        if (gameplay->depth[i] == 0)
            return FALSE;
    return TRUE;
}

/* Create a texture containing images of the keys to use for each
 * place's move command. The images are arranged in a 4x3 square in
 * the texture. (The "alternate" move command is used because keycaps
 * typically have uppercase letters.)
 */
static void makekeyguides(void)
{
    SDL_Color const keytextcolor = { 175, 175, 191, 0 };
    SDL_Color const keycolor = { 0, 0, 0, 0 };
    SDL_Surface *letters[MOVEABLE_PLACE_COUNT];
    SDL_Surface *image;
    SDL_Rect rect;
    int lettersize, i;

    lettersize = 0;
    for (i = 0 ; i < MOVEABLE_PLACE_COUNT ; ++i) {
        letters[i] = TTF_RenderGlyph_Shaded(_graph.largefont,
                                            placetomovecmd2(indextoplace(i)),
                                            keytextcolor, keycolor);
        if (lettersize < letters[i]->h)
            lettersize = letters[i]->h;
    }
    keyguidesize = lettersize + 2;
    rect.w = 4 * keyguidesize;
    rect.h = (MOVEABLE_PLACE_COUNT / 4) * keyguidesize;
    image = SDL_CreateRGBSurface(SDL_SWSURFACE, rect.w, rect.h,
                                 32, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_FillRect(image, NULL,
                 SDL_MapRGB(image->format, colors3(keytextcolor)));
    for (i = 0 ; i < MOVEABLE_PLACE_COUNT ; ++i) {
        rect.x = (i % 4) * keyguidesize + 1;
        rect.y = (i / 4) * keyguidesize + 1;
        rect.w = lettersize;
        rect.h = lettersize;
        SDL_FillRect(image, &rect,
                     SDL_MapRGB(image->format, colors3(keycolor)));
        rect.x += (lettersize - letters[i]->w) / 2;
        rect.y += (lettersize - letters[i]->h) / 2;
        rect.w = letters[i]->w;
        rect.h = letters[i]->h;
        SDL_BlitSurface(letters[i], NULL, image, &rect);
        SDL_FreeSurface(letters[i]);
    }
    keyguidetexture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(image);
}

/* Render all active overlays to the display.
 */
static void renderoverlays(void)
{
    int i;

    for (i = 0 ; i < (int)(sizeof overlays / sizeof *overlays) ; ++i) {
        if (!overlays[i].inuse)
            continue;
        if (overlays[i].card) {
            rendercard(overlays[i].card, overlays[i].pos.x, overlays[i].pos.y);
        } else if (overlays[i].icon) {
            renderalphaimage(overlays[i].icon, overlays[i].alpha,
                             overlays[i].pos.x, overlays[i].pos.y);
        } else if (overlays[i].texture) {
            SDL_SetTextureAlphaMod(overlays[i].texture, overlays[i].alpha);
            SDL_RenderCopy(_graph.renderer, overlays[i].texture, NULL,
                           &overlays[i].pos);
        }
    }
}

/* Render the background of the options dialog. (The contents of the
 * dialog are all buttons, so they are rendered automatically when
 * visible.)
 */
static void renderoptions(void)
{
    SDL_SetRenderDrawColor(_graph.renderer, colors4(_graph.bkgndcolor));
    SDL_RenderFillRect(_graph.renderer, &dialog);
    SDL_SetRenderDrawColor(_graph.renderer, colors4(_graph.defaultcolor));
    SDL_RenderDrawLines(_graph.renderer, dialogoutline,
                        sizeof dialogoutline / sizeof *dialogoutline);
}

/* Render indicators showing redoable moves from the given place, if
 * any. The indicators take the form of either the redoable move's
 * letter command, or, if the move is part of a complete solution, the
 * size of the solution it is part of. Additionally, if the card at
 * this place has a legal move, and showmoveable is true, then a dot
 * is drawn between the two indicators.
 */
static void rendernavinfo(gameplayinfo const *gameplay,
                          redo_position const *position,
                          int place, int yoffset, int showmoveable)
{
    redo_position const *firstpos;
    redo_position const *secondpos;
    redo_branch const *branch;
    char buf[2];
    SDL_Rect rect;
    int x, y, spacing;

    if (gameplay->locked & (1 << place))
        return;

    firstpos = NULL;
    secondpos = NULL;
    for (branch = position->next ; branch ; branch = branch->cdr) {
        if (branch->move == cardtomoveid1(gameplay->cardat[place]))
            firstpos = branch->p;
        else if (branch->move == cardtomoveid2(gameplay->cardat[place]))
            secondpos = branch->p;
    }

    spacing = dotsize.x / 2;
    x = placelocs[place].x + spacing;
    y = placelocs[place].y + yoffset + _graph.cardsize.y;
    if (firstpos) {
        if (firstpos->solutionsize) {
            drawsmallnumber(firstpos->solutionsize, x, y, +1);
        } else {
            buf[0] = placetomovecmd1(place);
            buf[1] = '\0';
            drawsmalltext(buf, x, y, +1);
        }
    }
    x = placelocs[place].x + _graph.cardsize.x - spacing;
    if (secondpos) {
        if (secondpos->solutionsize) {
            drawsmallnumber(secondpos->solutionsize, x, y, -1);
        } else {
            buf[0] = placetomovecmd2(place);
            buf[1] = '\0';
            drawsmalltext(buf, x, y, -1);
        }
    }

    if (showmoveable && gameplay->moveable & (1 << place)) {
        rect.x = placelocs[place].x + (_graph.cardsize.x - dotsize.x) / 2;
        rect.y = y;
        rect.w = dotsize.x;
        rect.h = dotsize.y;
        SDL_RenderCopy(_graph.renderer, dot, NULL, &rect);
    }
}

/* Output an indicator of whether a better seequence of moves exists
 * for the current position.
 */
static void renderbetterinfo(redo_position const *position)
{
    redo_position const *bp;
    SDL_Rect rect;
    int isbetter;

    if (!position || !position->better)
        return;

    for (bp = position ; bp->better ; bp = bp->better) ;
    if (bp->movecount < position->movecount) {
        isbetter = TRUE;
    } else if (bp->movecount == position->movecount && bp->solutionsize) {
        isbetter = !position->solutionsize ||
                        bp->solutionsize < position->solutionsize;
    } else {
        isbetter = FALSE;
    }
    if (!isbetter)
        return;

    settextcolor(bp->solutionsize ? _graph.defaultcolor : _graph.dimmedcolor);
    rect.x = bettercount.x - 2 * equalsize.x;
    rect.y = bettercount.y;
    rect.w = equalsize.x;
    rect.h = equalsize.y;
    rect.x -= drawsmallnumber(bp->movecount, bettercount.x, bettercount.y, -1);
    SDL_RenderCopy(_graph.renderer, equal, NULL, &rect);
    settextcolor(_graph.defaultcolor);
}

/* Render keycap images at the head of each place's location.
 */
static void renderkeyguides(void)
{
    SDL_Rect src, dest;
    int i;

    if (!keyguidetexture)
        makekeyguides();
    for (i = 0 ; i < MOVEABLE_PLACE_COUNT ; ++i) {
        src.x = (i % 4) * keyguidesize;
        src.y = (i / 4) * keyguidesize;
        src.w = keyguidesize;
        src.h = keyguidesize;
        dest.x = placelocs[i].x + 3 * (placelocs[i].w - keyguidesize) / 4;
        dest.y = placelocs[i].y - keyguidesize / 2;
        dest.w = keyguidesize;
        dest.h = keyguidesize;
        SDL_RenderCopy(_graph.renderer, keyguidetexture, &src, &dest);
    }
}

/* Render all the elements of the game's layout. The tableau stacks
 * require extra logic, since their sequence is stored from front to
 * back, but they need to be rendered back to front.
 */
static void renderlayout(void)
{
    card_t stack[42], card;
    int showmoveable, i, n;

    showmoveable = shouldmarkmoveable(gameplay);

    for (i = FOUNDATION_PLACE_1ST ; i < FOUNDATION_PLACE_END ; ++i)
        rendercard(gameplay->cardat[i], placelocs[i].x, placelocs[i].y);

    for (i = RESERVE_PLACE_1ST ; i < RESERVE_PLACE_END ; ++i) {
        if (!gameplay->depth[i]) {
            rendercard(EMPTY_PLACE, placelocs[i].x, placelocs[i].y);
            continue;
        }
        rendercard(gameplay->cardat[i], placelocs[i].x, placelocs[i].y);
        rendernavinfo(gameplay, position, i, 0, showmoveable);
    }

    for (i = TABLEAU_PLACE_1ST ; i < TABLEAU_PLACE_END ; ++i) {
        if (!gameplay->depth[i]) {
            rendercard(EMPTY_PLACE, placelocs[i].x, placelocs[i].y);
            continue;
        }
        card = gameplay->cardat[i];
        n = gameplay->depth[i];
        while (n--) {
            stack[n] = card;
            card = gameplay->covers[cardtoindex(card)];
        }
        for (n = 0 ; n < gameplay->depth[i] ; ++n)
            rendercard(stack[n],
                       placelocs[i].x, placelocs[i].y + n * _graph.dropheight);
        rendernavinfo(gameplay, position, i, (n - 1) * _graph.dropheight,
                      showmoveable);
    }
}

/* Render the counters and indicators of the sidebar. This includes
 * the "stuck" and "done" alerts, even though they appear outside of
 * the sidebar area.
 */
static void rendersidebar(void)
{
    drawlargenumber(position->movecount, movecount.x, movecount.y, -1);
    renderbetterinfo(position);
    if (gameplay->bestsolution) {
        drawlargenumber(gameplay->bestsolution, bestcount.x, bestcount.y, -1);
        drawlargenumber(bestknownsolutionsize(gameplay->gameid),
                        bestknowncount.x, bestknowncount.y, -1);
    }
    if (gameplay->endpoint)
        renderimage(IMAGE_DONE, status.x, status.y);
    else if (!gameplay->moveable)
        renderimage(IMAGE_STUCK, status.x, status.y);
    if (bookmarkflag)
        renderimage(IMAGE_BOOKMARK, bookmark.x, bookmark.y);
}

/* Render the complete game display, back to front.
 */
static void render(void)
{
    SDL_SetRenderDrawColor(_graph.renderer, colors4(tablecolor));
    SDL_RenderClear(_graph.renderer);
    if (!gameplay)
        return;
    renderlayout();
    if (keyschkbox.state & BSTATE_SELECT)
        renderkeyguides();
    rendersidebar();
    renderoverlays();
    if (optionsopen)
        renderoptions();
}

/*
 * Managing user input.
 */

/* Map keyboard input events to user commands.
 */
static command_t handlekeyevent(SDL_Keysym key)
{
    if (key.mod & (KMOD_ALT | KMOD_GUI | KMOD_MODE))
        return cmd_none;

    if (key.mod & KMOD_CTRL) {
        switch (key.sym) {
          case SDLK_o:          return cmd_changesettings;
          case SDLK_y:          return cmd_redo;
          case SDLK_z:          return cmd_undo;
        }
    } else if (key.mod & KMOD_SHIFT) {
        switch (key.sym) {
          case SDLK_m:          return cmd_pushbookmark;
          case SDLK_p:          return cmd_dropbookmark;
          case SDLK_r:          return cmd_popbookmark;
          case SDLK_s:          return cmd_swapbookmark;
        }
    } else {
        switch (key.sym) {
          case SDLK_SPACE:      return cmd_autoplay;
          case SDLK_LEFT:       return cmd_undo;
          case SDLK_RIGHT:      return cmd_redo;
          case SDLK_PAGEUP:     return cmd_undo10;
          case SDLK_PAGEDOWN:   return cmd_redo10;
          case SDLK_UP:         return cmd_undotobranch;
          case SDLK_DOWN:       return cmd_redotobranch;
          case SDLK_HOME:       return cmd_jumptostart;
          case SDLK_END:        return cmd_jumptoend;
          case SDLK_UNDO:       return cmd_undo;
          case SDLK_BACKSPACE:  return cmd_erase;
        }
    }
    return cmd_none;
}

/* Map ASCII characters to user commands. The commands handled here
 * are ones for actual characters, and so cannot be detected as key
 * events since keyboards can differ as to how a character is typed.
 */
static command_t handletextevent(char const *text)
{
    int i;

    for (i = 0 ; text[i] ; ++i) {
        if (ismovecmd(text[i]))
            return text[i];
        switch (text[i]) {
          case '=':             return cmd_switchtobetter;
          case '-':             return cmd_switchtoprevious;
          case '!':             return cmd_setminimalpath;
        }
    }
    return cmd_none;
}

/* Identify mouse events that correspond to card locations, and
 * translate them into move commands.
 */
static command_t handlemouseevent(SDL_Event const *event)
{
    int usefirst, p;

    if (event->button.button == SDL_BUTTON_RIGHT)
        usefirst = FALSE;
    else if (event->button.button == SDL_BUTTON_LEFT)
        usefirst = !(SDL_GetModState() & KMOD_SHIFT);
    else
        return cmd_none;

    if (event->button.y >= tableau.y) {
        for (p = TABLEAU_PLACE_1ST ; p < TABLEAU_PLACE_END ; ++p)
            if (event->button.x >= placelocs[p].x &&
                        event->button.x < placelocs[p].x + placelocs[p].w)
                return usefirst ? placetomovecmd1(p) : placetomovecmd2(p);
    } else {
        for (p = RESERVE_PLACE_1ST ; p < RESERVE_PLACE_END ; ++p)
            if (rectcontains(&placelocs[p], event->button.x, event->button.y))
                return usefirst ? placetomovecmd1(p) : placetomovecmd2(p);
    }

    return cmd_none;
}

/* Handle all the various types of input events.
 */
static command_t eventhandler(SDL_Event *event)
{
    switch (event->type) {
      case SDL_KEYDOWN:
        return handlekeyevent(event->key.keysym);
      case SDL_TEXTINPUT:
        return handletextevent(event->text.text);
      case SDL_MOUSEBUTTONDOWN:
        if (optionsopen) {
            if (!rectcontains(&dialog, event->button.x, event->button.y)) {
                setoptionsdisplay(FALSE);
                return cmd_changesettings;
            }
            return cmd_none;
        }
        return handlemouseevent(event);
    }
    return cmd_none;
}

/*
 * Internal functions.
 */

/* Function called when the game display is about to become the active
 * display. Any settings that might be lingering from a prior game are
 * reset.
 */
void resetgamedisplay(void)
{
    prevbestsolutionsize = -1;
    if (optionsopen)
        setoptionsdisplay(FALSE);
}

/* Store the latest parameters describing the game state.
 */
void updategamestate(gameplayinfo const *newgameplay,
                     redo_position const *newposition, int newbookmarkflag)
{
    gameplay = newgameplay;
    position = newposition;
    bookmarkflag = newbookmarkflag;
    if (prevbestsolutionsize < 0)
        prevbestsolutionsize = gameplay->bestsolution;
}

/* If display is true, show the options dialog and update the
 * checkboxes to match the current settings. If display is false, hide
 * the options dialog and update the current settings to match the
 * checkboxes.
 */
void showoptions(settingsinfo *settings, int display)
{
    if (settings) {
        if (display) {
            setbitflag(keyschkbox.state, BSTATE_SELECT, settings->showkeys);
            setbitflag(animchkbox.state, BSTATE_SELECT, settings->animation);
            setbitflag(autochkbox.state, BSTATE_SELECT, settings->autoplay);
            setbitflag(redochkbox.state, BSTATE_SELECT, settings->branching);
        } else {
            settings->showkeys = keyschkbox.state & BSTATE_SELECT;
            settings->animation = animchkbox.state & BSTATE_SELECT;
            settings->autoplay = autochkbox.state & BSTATE_SELECT;
            settings->branching = redochkbox.state & BSTATE_SELECT;
        }
    }
    setoptionsdisplay(display);
}

/* Initialize resources and return the game display's displaymap.
 */
displaymap initgamedisplay(void)
{
    int const dotchar = 0x2022;
    displaymap display;
    SDL_Surface *image;

    image = TTF_RenderGlyph_Blended(_graph.smallfont, '=',
                                    _graph.defaultcolor);
    equalsize.x = image->w;
    equalsize.y = image->h;
    equal = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(image);

    image = TTF_RenderGlyph_Blended(_graph.smallfont, dotchar,
                                    _graph.defaultcolor);
    dotsize.x = image->w;
    dotsize.y = image->h;
    dot = SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(image);

    makeimagebutton(&helpbutton, IMAGE_HELP);
    helpbutton.display = DISPLAY_GAME;
    helpbutton.cmd = cmd_showhelp;
    addbutton(&helpbutton);

    makeimagebutton(&backbutton, IMAGE_BACK);
    backbutton.display = DISPLAY_GAME;
    backbutton.cmd = cmd_quit;
    addbutton(&backbutton);

    makepopupbutton(&optionsbutton, IMAGE_OPTIONS);
    optionsbutton.display = DISPLAY_GAME;
    optionsbutton.cmd = cmd_changesettings;
    addbutton(&optionsbutton);

    makecheckbox(&keyschkbox, "Show move keys");
    keyschkbox.cmd = cmd_none;
    keyschkbox.display = DISPLAY_GAME;
    keyschkbox.visible = 0;
    addbutton(&keyschkbox);

    makecheckbox(&animchkbox, "Animate card movements");
    animchkbox.cmd = cmd_none;
    animchkbox.display = DISPLAY_GAME;
    animchkbox.visible = 0;
    addbutton(&animchkbox);

    makecheckbox(&autochkbox, "Auto-play on foundations");
    autochkbox.cmd = cmd_none;
    autochkbox.display = DISPLAY_GAME;
    autochkbox.visible = 0;
    addbutton(&autochkbox);

    makecheckbox(&redochkbox, "Enable branching redo");
    redochkbox.cmd = cmd_none;
    redochkbox.display = DISPLAY_GAME;
    redochkbox.visible = 0;
    addbutton(&redochkbox);

    display.setlayout = setlayout;
    display.render = render;
    display.eventhandler = eventhandler;
    return display;
}

/*
 * API functions.
 */

/* Change the key guide checkbox setting.
 */
int sdl_setshowkeyguidesflag(int flag)
{
    setbitflag(keyschkbox.state, BSTATE_SELECT, flag);
    return flag;
}

/* Change the animation setting, including the checkbox.
 */
int sdl_setcardanimationflag(int flag)
{
    cardanimation = flag;
    setbitflag(animchkbox.state, BSTATE_SELECT, flag);
    return flag;
}

/* Create an animation of a card moving across the display, invoking a
 * callback once the animation completes. If animations are turned
 * off, invoke the callback directly and return.
 */
void sdl_movecard(gameplayinfo const *gameplay, card_t card,
                  place_t from, place_t to,
                  void (*callback)(void*), void *data)
{
    if (cardanimation)
        createcardanimation(gameplay, card, from, to, callback, data);
    else
        (*callback)(data);
}

/* Temporarily display the save icon.
 */
void sdl_showsolutionwrite(void)
{
    createsaveanimation();
    if (prevbestsolutionsize)
        createreplacementanimation(prevbestsolutionsize);
}
