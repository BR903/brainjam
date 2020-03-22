/* list.c: the list selection display.
 */

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include "./gen.h"
#include "./types.h"
#include "./commands.h"
#include "./decks.h"
#include "solutions/solutions.h"
#include "internal.h"
#include "images.h"
#include "button.h"
#include "scroll.h"

/* The currently selected game ID.
 */
static int selection;

/* The various buttons that appear on the list display.
 */
static button helpbutton, quitbutton, playbutton, clipbutton;
static button prevbutton, randbutton, nextbutton;

/* The scrollbar for the selection list.
 */
static scrollbar scroll;

/* The locations of the list display elements.
 */
static SDL_Rect bannerrect;             /* location of the banner graphic */
static SDL_Rect headlinerect;           /* location of the headline text */
static SDL_Rect listrect;               /* location of the game list */
static SDL_Rect scorearea;              /* location of the score display */
static SDL_Point scorelabel;            /* position of score label text */
static SDL_Point scorenumber;           /* position of score */
static SDL_Point markersize;            /* size of game list markers */
static int markeroffset;                /* vertical offset of list markers */
static int rowheight;                   /* height of one list entry */

/* The texture of the banner graphic.
 */
static SDL_Texture *bannertexture;

/* The texture of the banner graphic's headline.
 */
static SDL_Texture *headlinetexture;

/*
 * Managing the list's selection and scroll position.
 */

/* Clamp the given value to the scrollbar range.
 */
static int clampscrollpos(int n)
{
    if (n < 0)
        return 0;
    if (n >= scroll.range)
        return scroll.range - 1;
    return n;
}

/* Change the currently selected game, updating the state of the
 * clipboard button in the process.
 */
static void changeselection(int id)
{
    selection = id;
    clipbutton.state = getsolutionfor(id) ? BSTATE_NORMAL : BSTATE_DISABLED;
}

/* Choose a scroll position for the list that ensures that the current
 * selection is visible. (Or mostly visible at least -- it's okay for
 * it to be partially off the edge.)
 */
static int normalizeposition(void)
{
    int top, bottom;

    top = scroll.value / rowheight;
    if (scroll.value % rowheight > rowheight / 3)
        ++top;
    bottom = (scroll.value + scroll.pagesize) / rowheight - 1;
    if ((scroll.value + scroll.pagesize) % rowheight > rowheight / 2)
        ++bottom;
    if (selection < top)
        return clampscrollpos(selection * rowheight);
    if (selection > bottom)
        return clampscrollpos(selection * rowheight - scroll.pagesize
                                                    + rowheight);
    return scroll.value;
}

/* If the selection is currently not visible, move it to one of the
 * rows that is visible. (Note that changing the selection can
 * sometimes cause the list to scroll, thus altering visibility. To
 * avoid the risk of a feedback loop causing jitter or twitching, this
 * function updates the selection using the lower-level
 * changeselection() function instead of setselection().)
 */
static void normalizeselection(void)
{
    int pos;

    pos = selection * rowheight;
    if (pos < scroll.value - rowheight / 3) {
        pos = scroll.value + 2 * rowheight / 3;
        changeselection(pos / rowheight);
    } else if (pos > scroll.value + scroll.pagesize - rowheight / 2) {
        pos = scroll.value + scroll.pagesize - rowheight / 2;
        changeselection(pos / rowheight);
    }
}

/* Callback that is invoked after a scrolling motion completes, to
 * keep the selection on a visible row.
 */
static void scrollfinish(void *data, int done)
{
    (void)data;
    (void)done;
    normalizeselection();
}

/* Scroll the list to the given position, using an animation to make
 * the movement more smooth.
 */
static int scrolltoposition(int position)
{
    animinfo *anim;

    position = clampscrollpos(position);
    if (position == scroll.value)
        return FALSE;
    anim = getaniminfo();
    anim->steps = 8;
    anim->pval1 = &scroll.value;
    anim->destval1 = position;
    anim->pval2 = NULL;
    anim->callback = scrollfinish;
    anim->data = NULL;
    startanimation(anim, 10);
    return TRUE;
}

/* Change the current selection, forcing it into the range of valid
 * games. The scrolling list is updated if necessary so that the new
 * selection is visible. Returns cmd_redraw, even if the selection is
 * unchanged.
 */
static command_t setselection(int id)
{
    int count;

    count = getdeckcount();
    if (id < 0)
        id = 0;
    else if (id >= count)
        id = count - 1;
    changeselection(id);
    scrolltoposition(normalizeposition());
    return cmd_redraw;
}

/*
 * Placement of display elements.
 */

/* Calculate the locations and positions of the various elements that
 * make up the list display: The banner on top, the quit button in one
 * corner and the help button in the other, the list near the middle
 * with its associated header text and button controls, the play
 * button under the list, and the score area to the right of the list.
 */
static SDL_Point setlayout(SDL_Point display)
{
    SDL_Rect area;
    SDL_Point size;
    int textheight, w;

    quitbutton.pos.x = _graph.margin;
    quitbutton.pos.y = display.y - quitbutton.pos.h - _graph.margin;

    helpbutton.pos.x = display.x - helpbutton.pos.w - _graph.margin;
    helpbutton.pos.y = quitbutton.pos.y - helpbutton.pos.h - _graph.margin;

    bannerrect.x = 0;
    bannerrect.y = 0;
    headlinerect.x = display.x < bannerrect.w ? display.x : bannerrect.w;
    headlinerect.x -= headlinerect.w;
    headlinerect.y = bannerrect.y + bannerrect.h - headlinerect.h;

    area.x = quitbutton.pos.w + 2 * _graph.margin;
    area.w = (display.x / 2) - area.x;
    area.y = bannerrect.y + bannerrect.h + _graph.margin;
    area.h = display.y - area.y - _graph.margin;

    rowheight = TTF_FontLineSkip(_graph.largefont);
    if (rowheight < markersize.y)
        rowheight = markersize.y;
    markeroffset = TTF_FontAscent(_graph.largefont) - markersize.y;

    TTF_SizeUTF8(_graph.largefont, " 8888 ", &w, NULL);
    TTF_SizeUTF8(_graph.largefont, " Game ", &size.x, NULL);
    if (w < size.x)
        w = size.x;
    w += 2 * markersize.x;
    if (w < playbutton.pos.w)
        w = playbutton.pos.w;
    listrect.x = area.x + (area.w - listrect.w) / 2;
    listrect.y = area.y + rowheight + 2;
    listrect.w = w;
    listrect.h = area.y + area.h - listrect.y;
    listrect.h -= _graph.margin + playbutton.pos.h;

    prevbutton.pos.x = listrect.x - prevbutton.pos.w - _graph.margin;
    prevbutton.pos.y = listrect.y + (listrect.h - 3 * prevbutton.pos.h) / 2;
    randbutton.pos.x = prevbutton.pos.x;
    randbutton.pos.y = prevbutton.pos.y + prevbutton.pos.h;
    nextbutton.pos.x = randbutton.pos.x;
    nextbutton.pos.y = randbutton.pos.y + randbutton.pos.h;

    playbutton.pos.x = listrect.x + (listrect.w - playbutton.pos.w) / 2;
    playbutton.pos.y = area.y + area.h - playbutton.pos.h;

    clipbutton.pos.x = playbutton.pos.x + playbutton.pos.w + 2 * _graph.margin;
    clipbutton.pos.y = playbutton.pos.y;

    scroll.pos.x = listrect.x + listrect.w + _graph.margin / 2;
    scroll.pos.y = listrect.y;
    scroll.pos.w = _graph.margin / 2;
    scroll.pos.h = listrect.h;
    scroll.pagesize = listrect.h;
    scroll.linesize = rowheight;
    scroll.range = getdeckcount() * rowheight - listrect.h + 1;
    scroll.value = clampscrollpos(scroll.value);
    normalizeselection();

    scorearea.x = display.x / 2 + _graph.margin;
    scorearea.y = area.y + 2 * _graph.margin;
    scorearea.w = display.x / 2 - area.x - helpbutton.pos.w;
    scorearea.w -= 2 * _graph.margin;

    textheight = TTF_FontLineSkip(_graph.smallfont);
    TTF_SizeUTF8(_graph.smallfont, "Your best solution:", &w, NULL);
    scorelabel.x = w;
    scorelabel.y = scorearea.y;
    TTF_SizeUTF8(_graph.smallfont, "Best possible:", &w, NULL);
    if (scorelabel.x < w)
        scorelabel.x = w;
    scorelabel.x += scorearea.x;
    TTF_SizeUTF8(_graph.smallfont, " 888", &w, NULL);
    scorenumber.x = scorelabel.x + w;
    scorenumber.y = scorearea.y;
    scorearea.h = 4 * textheight;

    size.x = listrect.w + scorearea.w + quitbutton.pos.w + helpbutton.pos.w;
    size.x += 6 * _graph.margin;
    size.y = bannerrect.h + 8 * rowheight + playbutton.pos.h;
    size.y += 4 * _graph.margin;
    return size;
}

/*
 * Rendering the display.
 */

/* Render the components of the score area. If the currently selected
 * game has a stored solution, then the score area is updated to show
 * the number of moves in the solution, and the smallest possible
 * solution. If the user has no solutions, then it is assumed that
 * they are new to the game, and some helpful text is shown in this
 * space instead.
 */
static void renderscorearea(int id)
{
    static char const *directions[] = {
        "Select one of the games",
        "from the list and press play.",
        "Or use the spin button",
        "to select a random game."
    };
    solutioninfo const *solution;
    char buf[8];
    int h, i;

    solution = getsolutionfor(id);

    h = TTF_FontLineSkip(_graph.smallfont);
    if (getsolutioncount() < 1) {
        for (i = 0 ; i < (int)(sizeof directions / sizeof *directions) ; ++i)
            drawsmalltext(directions[i], scorearea.x, scorearea.y + i * h, -1);
    } else if (solution) {
        drawsmalltext("Your best solution:", scorelabel.x, scorelabel.y, +1);
        sprintf(buf, "%d", solution->size);
        drawsmalltext(buf, scorenumber.x, scorenumber.y, +1);
        drawsmalltext("Best possible:", scorelabel.x, scorelabel.y + h, +1);
        sprintf(buf, "%d", bestknownsolutionsize(id));
        drawsmalltext(buf, scorenumber.x, scorenumber.y + h, +1);
    }
}

/* Render one entry in the list of games. ypos specifies the vertical
 * offset from the top of the list.
 */
static void renderlistentry(int id, int ypos)
{
    solutioninfo const *solution;
    SDL_Rect rect;
    char buf[8];

    rect.x = listrect.x;
    rect.y = listrect.y + ypos;
    rect.w = listrect.w;
    rect.h = rowheight;

    if (id == selection) {
        SDL_SetRenderDrawColor(_graph.renderer,
                               colors4(_graph.lightbkgndcolor));
        SDL_RenderFillRect(_graph.renderer, &rect);
        SDL_SetRenderDrawColor(_graph.renderer, colors4(_graph.defaultcolor));
        settextcolor(_graph.highlightcolor);
    } else {
        settextcolor(_graph.defaultcolor);
    }

    sprintf(buf, "%04d", id);
    drawlargetext(buf, rect.x + rect.w / 2, rect.y, 0);

    solution = getsolutionfor(id);
    if (solution && solution->size <= bestknownsolutionsize(id)) {
        renderimage(IMAGE_STAR, rect.x, rect.y + markeroffset);
        renderimage(IMAGE_STAR,
                    rect.x + rect.w - markersize.x, rect.y + markeroffset);
    } else if (solution) {
        renderimage(IMAGE_CHECK, rect.x, rect.y + markeroffset);
    }
}

/* Render the list of games. First the entries are draw, then the
 * header text and the top and bottom lines, and finally the scrollbar
 * on the right.
 */
static void rendergamelist(void)
{
    int id, y;

    SDL_RenderSetClipRect(_graph.renderer, &listrect);
    id = scroll.value / rowheight;
    y = -(scroll.value % rowheight);
    for ( ; y < listrect.y + listrect.h ; ++id, y += rowheight)
        renderlistentry(id, y);
    SDL_RenderSetClipRect(_graph.renderer, NULL);

    settextcolor(_graph.defaultcolor);
    drawlargetext("Game",
                  listrect.x + listrect.w / 2, listrect.y - rowheight - 2, 0);
    SDL_SetRenderDrawColor(_graph.renderer, colors4(_graph.defaultcolor));
    SDL_RenderDrawLine(_graph.renderer, listrect.x, listrect.y - 1,
                       listrect.x + listrect.w, listrect.y - 1);
    SDL_RenderDrawLine(_graph.renderer,
                       listrect.x, listrect.y + listrect.h + 1,
                       listrect.x + listrect.w, listrect.y + listrect.h + 1);
    scrollrender(&scroll);
}

/* Render all components of the of the display: the banner, the game
 * list, and the score area.
 */
static void render(void)
{
    SDL_SetRenderDrawColor(_graph.renderer, colors4(_graph.bkgndcolor));
    SDL_RenderClear(_graph.renderer);
    SDL_RenderCopy(_graph.renderer, bannertexture, NULL, &bannerrect);
    SDL_RenderCopy(_graph.renderer, headlinetexture, NULL, &headlinerect);
    rendergamelist();
    renderscorearea(selection);
}

/*
 * UI element callback functions.
 */

/* Move the current selection to the previous unsolved game.
 */
static void selectbackward(int down)
{
    (void)down;
    setselection(findnextunsolved(selection, -1));
}

/* Move the current selection to the next unsolved game.
 */
static void selectforward(int down)
{
    (void)down;
    setselection(findnextunsolved(selection, +1));
}

/* Move the current selection to a random unsolved game.
 */
static void selectrandom(int down)
{
    (void)down;
    setselection(pickrandomunsolved());
}

/* Copy the text of the selected game's solution to the clipboard.
 */
static void copyselectedsolution(int down)
{
    solutioninfo const *solution;

    (void)down;
    solution = getsolutionfor(selection);
    if (solution)
        SDL_SetClipboardText(solution->text);
}

/*
 * Managing user input.
 */

/* Change the current selection by a relative amount.
 */
static command_t moveselection(int delta)
{
    return setselection(selection + delta);
}

/* Map keyboard input events to user commands.
 */
static command_t handlekeyevent(SDL_Keysym key)
{
    if (key.mod & (KMOD_ALT | KMOD_GUI | KMOD_MODE))
        return cmd_none;

    if (key.mod & KMOD_CTRL) {
        if (key.sym == SDLK_r) {
            selectrandom(0);
            return cmd_redraw;
        } else if (key.sym == SDLK_c) {
            copyselectedsolution(0);
            return cmd_redraw;
        }
        return cmd_none;
    }

    switch (key.sym) {
      case SDLK_UP:       return moveselection(-1);
      case SDLK_DOWN:     return moveselection(+1);
      case SDLK_PAGEUP:   return moveselection(-scroll.pagesize / rowheight);
      case SDLK_PAGEDOWN: return moveselection(+scroll.pagesize / rowheight);
      case SDLK_HOME:     return setselection(0);
      case SDLK_END:      return setselection(getdeckcount());
      case SDLK_RETURN:   return cmd_select;
      case SDLK_KP_ENTER: return cmd_select;
      case SDLK_TAB:
        if (key.mod & KMOD_SHIFT)
            selectbackward(0);
        else
            selectforward(0);
        return cmd_redraw;
    }
    return cmd_none;
}

/* The list display mainly listens for events that interact with the
 * scrolling list of games. The return value is the command to return
 * to the calling code, or zero if the command was not handled by this
 * code. A return value of cmd_select is handled by the selectgame()
 * function, and indicates that the currently selected game should be
 * initiated.
 */
static command_t eventhandler(SDL_Event *event)
{
    int x, y;

    if (scrolleventhandler(event, &scroll)) {
        normalizeselection();
        return cmd_redraw;
    }
    switch (event->type) {
      case SDL_KEYDOWN:
        return handlekeyevent(event->key.keysym);
      case SDL_MOUSEBUTTONDOWN:
        x = event->button.x;
        y = event->button.y;
        if (rectcontains(&listrect, x, y)) {
            y -= listrect.y;
            setselection((scroll.value + y) / rowheight);
            return event->button.clicks > 1 ? cmd_select : cmd_redraw;
        }
        break;
      case SDL_MOUSEWHEEL:
        SDL_GetMouseState(&x, &y);
        if (rectcontains(&listrect, x, y)) {
            y = scroll.value - event->wheel.y * rowheight;
            scroll.value = clampscrollpos(y);
            normalizeselection();
            return cmd_redraw;
        }
        break;
    }
    return cmd_none;
}

/*
 * Internal functions.
 */

/* Return the currently selected game ID.
 */
int getselection(void)
{
    return selection;
}

/* Change the list selection to the given game ID. This is typically
 * called just before the list display is made active.
 */
void initlistselection(int id)
{
    setselection(id);
    scroll.value = normalizeposition();
}

/* Initialize resources and return the list display's displaymap.
 */
displaymap initlistdisplay(void)
{
    displaymap display;

    makeimagebutton(&helpbutton, IMAGE_HELP);
    helpbutton.display = DISPLAY_LIST;
    helpbutton.cmd = cmd_showhelp;
    addbutton(&helpbutton);

    makeimagebutton(&quitbutton, IMAGE_QUIT);
    quitbutton.display = DISPLAY_LIST;
    quitbutton.cmd = cmd_quit;
    addbutton(&quitbutton);

    makeimagebutton(&prevbutton, IMAGE_PREV);
    prevbutton.display = DISPLAY_LIST;
    prevbutton.action = selectbackward;
    addbutton(&prevbutton);

    makeimagebutton(&randbutton, IMAGE_RANDOM);
    randbutton.display = DISPLAY_LIST;
    randbutton.action = selectrandom;
    addbutton(&randbutton);

    makeimagebutton(&nextbutton, IMAGE_NEXT);
    nextbutton.display = DISPLAY_LIST;
    nextbutton.action = selectforward;
    addbutton(&nextbutton);

    makeimagebutton(&playbutton, IMAGE_PLAY);
    playbutton.display = DISPLAY_LIST;
    playbutton.cmd = cmd_select;
    addbutton(&playbutton);

    makeimagebutton(&clipbutton, IMAGE_CLIP);
    clipbutton.display = DISPLAY_LIST;
    clipbutton.action = copyselectedsolution;
    addbutton(&clipbutton);

    loadsplashgraphics(&bannertexture, &headlinetexture);
    SDL_QueryTexture(bannertexture, NULL, NULL, &bannerrect.w, &bannerrect.h);
    SDL_QueryTexture(headlinetexture, NULL, NULL,
                     &headlinerect.w, &headlinerect.h);
    markersize.x = getimagewidth(IMAGE_STAR);
    markersize.y = getimageheight(IMAGE_STAR);

    display.setlayout = setlayout;
    display.render = render;
    display.eventhandler = eventhandler;
    return display;
}
