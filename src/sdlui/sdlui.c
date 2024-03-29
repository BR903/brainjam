/* sdlui/sdlui.c: the graphical user interface, implemented using SDL.
 */

#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include "./gen.h"
#include "./types.h"
#include "./commands.h"
#include "./glyphs.h"
#include "./settings.h"
#include "./ui.h"
#include "redo/redo.h"
#include "files/files.h"
#include "game/game.h"
#include "sdlui/sdlui.h"
#include "internal.h"
#include "sdltypes.h"
#include "image.h"
#include "button.h"
#include "font.h"

/* The list of displays that the game provides. Each display is
 * assigned an ID number, which it can use to control the display of
 * its buttons.
 */
#define DISPLAY_NONE   0        /* no active display */
#define DISPLAY_LIST   1        /* the game list and splash screen */
#define DISPLAY_GAME   2        /* the game proper */
#define DISPLAY_HELP   3        /* the help screens */
#define DISPLAY_COUNT  4

/* Convenience macro for initializing an SDL_Color struct.
 */
#define setcolor(c, rr, gg, bb) \
    ((c).r = (rr), (c).g = (gg), (c).b = (bb), (c).a = SDL_ALPHA_OPAQUE)

/* The types of timer-initiated events.
 */
enum {
    event_none = 0,             /* no event, or event was already handled */
    event_redraw,               /* repaint the window */
    event_unget,                /* inject a command into the input stream */
    event_animate,              /* update an animation */
    event_count
};

/* The data accompanying timer-initiated events.
 */
typedef struct timedeventinfo {
    int         type;           /* which type of event */
    SDL_TimerID timerid;        /* timer initiating events */
    int         interval;       /* interval for next event, or 0 to stop */
    void       *data;           /* caller-defined pointer */
} timedeventinfo;

/* An entry in the cache of rendered numbers, used by drawnumber().
 */
typedef struct numbercacheinfo {
    int         number;         /* integer value rendered */
    TTF_Font   *font;           /* font used in rendering */
    SDL_Color   color;          /* color used in rendering */
    SDL_Texture *texture;       /* the rendered glyphs */
    int         w;              /* width of cached texture */
    int         h;              /* height of cached texture */
} numbercacheinfo;

/* The help text for the initial list display. Since this might be the
 * first instructions that the user sees, it tries to be a little
 * extra friendly.
 */
static char const *listcommandshelptitle = "Selecting a Game";
static char const *listcommandshelptext =
    "Brain Jam contains hundreds of pre-shuffled layouts, each of which is"
    " guaranteed to be solvable. Each game is identified by a number, shown in"
    " the list in the center.\n"
    "\n"
    "Scroll through the list to select a game, and then press the play button"
    " to start playing that game.\n"
    "\n"
    "Games that you have already solved are marked in the list with a check"
    " mark. (Stars mark games for which you have found the shortest possible"
    " answer.)\n"
    "\n"
    "To the left of the list are up and down buttons. These buttons move the"
    " current selection up and down, but skip over games that you have already"
    " solved. The middle button, marked with a spinner, will move the"
    " selection to a random unsolved game. (You can also use the Tab key to"
    " move the selection up and down, and Ctrl-R to select a random game.)\n"
    "\n"
    "The clipboard button to the right will be enabled when you select a game"
    " that you have solved. Pressing it will copy your answer to the"
    " clipboard. The answer is represented as a series of A-L letters, each"
    " letter representing a move.";

/* The help text for basic game play.
 */
static char const *gameplayhelptitle = "How to Play";
static char const *gameplayhelptext =
    "To move a card, simply click on it with the left mouse button. If the"
    " card has a legal move, it will be made.\n"
    "\n"
    "If there is more than one legal move, the program will choose one. The"
    " program will generally prefer moves that maximize your options (such as"
    " moving a card onto a empty tableau column rather than an empty reserve,"
    " since the former can also be built upon).\n"
    "\n"
    "You can select an alternate destination for your move by using the right"
    " mouse button instead, or by holding down the Shift key while clicking"
    " the left mouse button.\n"
    "\n"
    "Alternately, you can move cards using the keyboard. Tableau cards can be"
    " moved with the letter keys A through H, for the eight columns going from"
    " left to right. Cards in the reserve can be moved with the letter keys I"
    " through L, again going from left to right. As with the mouse, you can"
    " select an alternate destination by holding down Shift while pressing a"
    " letter key.\n"
    "\n"
    "These letters will also appear underneath cards after undoing a move, to"
    " indicate which move the redo command will execute. A move is represented"
    " as a lower case letter on the left-hand side, or as an uppercase letter"
    " on the right-hand side for a move to the alternate destination.\n"
    "\n"
    "As you are playing, the current number of moves is displayed in the top"
    " right corner.\n"
    "\n"
    "If at any time no legal moves are available, a U-turn icon will appear at"
    " the top of the layout, and you will need to use undo in order to"
    " proceed. When you complete a game, a checkered-flag icon will appear"
    " instead. (You can use undo in this situation as well, if you wish to try"
    " to improve your answer. Otherwise, just use the back button in the"
    " bottom right corner to return to the game selection display.)\n"
    "\n"
    "If you are playing a game that you have already solved, then the number"
    " of moves in your answer will be displayed at bottom right, so that you"
    " can see the number you are trying to beat. Directly below that, the"
    " number of moves in the shortest possible answer is shown, so that you"
    " can also see how much room there is for improvement.\n"
    "\n"
    "The button with the gear icon will open a popup box that will allow you"
    " to change some of the game's settings. You can turn card movement on or"
    " off, and some other cosmetic features. You can also turn on the"
    " branching redo feature from here.";

/* The help text for the basic game key commands.
 */
static char const *gamecommandshelptitle = "Key Commands";
static char const *gamecommandshelptext =
    "Move top card from a tableau column\tA B C D E F G H\n"
    "Move a reserve card\tI J K L\n"
    "Move card to alternate spot\tShift-A ... Shift-L\n"
    "Undo previous move\t" GLYPH_LEFTARROW " or Ctrl-Z\n"
    "Redo next move\t" GLYPH_RIGHTARROW " or Ctrl-Y\n"
    "Undo to the starting position\tHome\n"
    "Redo all undone moves\tEnd\n"
    "Return to the previously viewed position\t" GLYPH_DASH "\n"
    "Display the options menu\tCtrl-O\n"
    "Display this help\t? or F1\n"
    "Quit and select a new layout\tQ or Esc\n"
    "Quit and exit the program\tShift-Q";

/* The help text for the branching redo key commands.
 */
static char const *redocommandshelptitle = "Redo Key Commands";
static char const *redocommandshelptext =
    "Undo previous move\t" GLYPH_LEFTARROW "\n"
    "Redo next move\t" GLYPH_RIGHTARROW "\n"
    "Undo and forget previous move\tBkspc\n"
    "Undo previous 10 moves\tPgUp\n"
    "Redo next 10 moves\tPgDn\n"
    "Undo backward to previous branch point\t" GLYPH_UPARROW "\n"
    "Redo forward to next branch point\t" GLYPH_DOWNARROW "\n"
    "Set redo moves to shortest answer\t!\n"
    "Switch to \"better\" position\t=\n"
    "Bookmark the current position\tShift-M\n"
    "Forget the last bookmarked position\tShift-P\n"
    "Restore the last bookmarked position\tShift-R\n"
    "Swap with the last bookmarked position\tShift-S";

/* Our window.
 */
static SDL_Window *window;

/* The set of callbacks for each of the game's displays.
 */
static displaymap displays[DISPLAY_COUNT];

/* Which display is currently active.
 */
static int displayed = DISPLAY_NONE;

/* True when the display needs to be updated.
 */
static int updatedisplay = TRUE;

/* True if a ding is pending.
 */
static int dinging = FALSE;

/* The complete list of buttons.
 */
static button **buttons = NULL;
static int buttoncount = 0;

/* The current text color.
 */
static SDL_Color currentcolor;

/* The ID value for timer-triggered events.
 */
static Uint32 timedeventid;

/* The global variable holding the shared graphics rendering objects.
 */
graphicinfo _graph;

/*
 * The do-nothing display, a placeholder used for transitional states.
 */

static SDL_Point none_setlayout(SDL_Point size)
{
    if (size.x < 1)
        size.x = 1;
    if (size.y < 1)
        size.y = 1;
    return size;
}

static void none_render(void)
{
}

static command_t none_eventhandler(SDL_Event *event)
{
    (void)event;
    return cmd_none;
}

static displaymap initnonedisplay(int id)
{
    displaymap display;

    (void)id;
    display.setlayout = none_setlayout;
    display.render = none_render;
    display.eventhandler = none_eventhandler;
    return display;
}

/*
 * Error messages.
 */

/* Display the current SDL error message in a popup and return false.
 */
static int err(char const *title)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title,
                             SDL_GetError(), window);
    return FALSE;
}

/* Display a bare-bones error message before closing the program. This
 * is reserved for assertion-level situations that should never happen.
 */
static void panic(char const *message)
{
    warn("fatal error: %s", message);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error",
                             message, window);
    exit(EXIT_FAILURE);
}

/* Store a texture to be destroyed at a later time. This deferral of
 * texture-freeing is done to work around a bug in SDL 2.0.10. This
 * bug was fixed in SDL 2.0.12, so at some point this function could
 * be removed, and calls to it replaced by direct calls to
 * SDL_DestroyTexture().
 */
static void releasetexture(SDL_Texture *texture)
{
    static SDL_Texture **texturedump = NULL;
    static int texturedumpsize = 0;
    static int texturecount = 0;
    int i;

    if (!texture) {
        for (i = 0 ; i < texturecount ; ++i)
            SDL_DestroyTexture(texturedump[i]);
        texturecount = 0;
        return;
    }

    if (texturecount >= texturedumpsize) {
        texturedumpsize = texturecount + 1;
        texturedump = reallocate(texturedump,
                                 texturedumpsize * sizeof *texturedump);
    }
    texturedump[texturecount++] = texture;
}

/* Proceed with the deferred texture cleanup. This should be invoked
 * after calling SDL_RenderPresent().
 */
#define garbagecollecttextures()  (releasetexture(NULL))

/*
 * Initialization of resources.
 */

/* Free resources stored in the global rendering object and close down
 * the UI.
 */
static void shutdown(void)
{
    if (TTF_WasInit()) {
        if (_graph.smallfont) {
            TTF_CloseFont(_graph.smallfont);
            _graph.smallfont = NULL;
        }
        if (_graph.largefont) {
            TTF_CloseFont(_graph.largefont);
            _graph.largefont = NULL;
        }
        TTF_Quit();
    }
    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        if (_graph.renderer) {
            SDL_DestroyRenderer(_graph.renderer);
            _graph.renderer = NULL;
        }
        SDL_Quit();
    }
}

/* Initialize the SDL libraries. Create the window and renderer, and
 * set the application's icon.
 */
static int startup(void)
{
    SDL_Surface *image;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER))
        return err("SDL_Init");
    if (TTF_Init())
        return err("TTF_Init");
    atexit(shutdown);

    window = SDL_CreateWindow("Brian Jam",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              512, 512, SDL_WINDOW_RESIZABLE);
    if (!window)
        return err("SDL_CreateWindow");
    _graph.renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!_graph.renderer)
        return err("SDL_CreateRenderer");

    timedeventid = SDL_RegisterEvents(1);
    SDL_StartTextInput();

    initializeimages();
    image = getbuttonlabel(IMAGE_ICON);
    SDL_SetWindowIcon(window, image);
    SDL_FreeSurface(image);
    return TRUE;
}

/* Given a screen size, coordinate the layout out of the UI elements
 * of each of the game's displays. If any layout indicates a minimum
 * working size larger that the given size, then that larger size is
 * adopted instead, and is returned.
 */
static SDL_Point measurelayout(SDL_Point size)
{
    SDL_Point bestsize, fullsize;
    int i;

    fullsize = size;

    for (i = 0 ; i < DISPLAY_COUNT ; ++i) {
        bestsize = displays[i].setlayout(size);
        if (fullsize.x < bestsize.x)
            fullsize.x = bestsize.x;
        if (fullsize.y < bestsize.y)
            fullsize.y = bestsize.y;
    }

    if (fullsize.x != size.x || fullsize.y != size.y)
        for (i = 0 ; i < DISPLAY_COUNT ; ++i)
            displays[i].setlayout(fullsize);

    return fullsize;
}

/* Create the program's fonts from the given font name, using the
 * playing card drop height to select appropriate character sizes. If
 * fontname is NULL, the GUI's default font will be used. Note that
 * the fontref object needs to be preserved for the lifetime of the
 * fonts, but since the font lifetime is just the lifetime of the
 * program, this is done by intentionally leaking the pointer.
 */
static int createfonts(char const *fontname)
{
    fontrefinfo *fontref;
    int smallfontsize, largefontsize;

    fontref = findnamedfont(fontname);
    if (!fontref) {
        SDL_SetError("Unable to locate a font with the name \"%s\"", fontname);
        return err("Font Lookup");
    }

    smallfontsize = (_graph.dropheight + 1) / 2;
    largefontsize = smallfontsize + (_graph.dropheight + 4) / 8;

    _graph.smallfont = getfontfromref(fontref, smallfontsize);
    if (!_graph.smallfont)
        return err(fontname);
    _graph.largefont = getfontfromref(fontref, largefontsize);
    if (!_graph.largefont)
        return err(fontname);

    return TRUE;
}

/* Complete the setting up of the program's window. Allocate our
 * fonts, which are used to size the layout grid for the displays.
 * After each display has initialized their own resources, they will
 * determine their own layouts, and from this the overall window size
 * is chosen and the window is made visible. (Note that if no initial
 * size is stored in the initialization file, then the program will
 * default to the minimum size, plus a little extra.)
 */
static int createdisplays(void)
{
    char const *ws;
    char const *hs;
    SDL_Point displaysize;

    setcolor(_graph.defaultcolor, 0, 0, 0);
    setcolor(_graph.dimmedcolor, 96, 96, 96);
    setcolor(_graph.highlightcolor, 32, 80, 255);
    setcolor(_graph.bkgndcolor, 191, 191, 191);
    setcolor(_graph.lightbkgndcolor, 213, 213, 213);
    settextcolor(_graph.defaultcolor);

    if (!createfonts(lookupinitsetting("font")))
        return FALSE;

    _graph.margin = TTF_FontLineSkip(_graph.smallfont);

    displays[DISPLAY_NONE] = initnonedisplay(DISPLAY_NONE);
    displays[DISPLAY_LIST] = initlistdisplay(DISPLAY_LIST);
    displays[DISPLAY_GAME] = initgamedisplay(DISPLAY_GAME);
    displays[DISPLAY_HELP] = inithelpdisplay(DISPLAY_HELP);

    ws = lookupinitsetting("windowwidth");
    hs = lookupinitsetting("windowheight");
    if (!ws || sscanf(ws, "%u", &displaysize.x) != 1)
        displaysize.x = 1;
    if (!hs || sscanf(hs, "%u", &displaysize.y) != 1)
        displaysize.y = 1;

    displaysize = measurelayout(displaysize);

    if (!ws && !hs)
        displaysize.y += 2 * _graph.dropheight;
    SDL_SetWindowSize(window, displaysize.x, displaysize.y);

    SDL_ShowWindow(window);
    return TRUE;
}

/* Store the window's size in the initialization file settings.
 */
static void recordwindowsize(SDL_Point size)
{
    char buf[16];

    sprintf(buf, "%u", size.x);
    storeinitsetting("windowwidth", buf);
    sprintf(buf, "%u", size.y);
    storeinitsetting("windowheight", buf);
}

/*
 * Timed events.
 */

/* Return a pointer to an unused timedeventinfo struct. Timer ticks
 * sometimes arrive just after removing the timer, so timedeventinfo
 * structs should not be freed or reused immediately after the timer
 * is removed. This pool allows timedeventinfo structs to linger for a
 * while after they have been ostensibly freed.
 */
static timedeventinfo *gettimedeventinfo(void)
{
    static timedeventinfo pool[16];
    static int lastindex = 0;
    int i;

    i = lastindex;
    for (;;) {
        i = (i + 1) % (sizeof pool / sizeof *pool);
        if (!pool[i].type) {
            lastindex = i;
            memset(pool + i, 0, sizeof *pool);
            break;
        }
        if (i == lastindex)
            panic("timedeventinfo pool exhausted");
    }
    return &pool[i];
}

/* Callback to transform a timer tick into a user-defined SDL event.
 */
static Uint32 timercallback(Uint32 interval, void *param)
{
    SDL_Event event;
    timedeventinfo *timedevent;

    (void)interval;
    timedevent = param;
    event.user.type = timedeventid;
    event.user.code = timedevent->type;
    event.user.data1 = param;
    SDL_PushEvent(&event);
    return timedevent->interval;
}

/* Schedule a timed event to be delivered after a delay, measured in
 * milliseconds. The which parameter specifies the event type.
 */
static void scheduletimedevent(int which, int delay, void *data)
{
    timedeventinfo *timedevent;

    timedevent = gettimedeventinfo();
    timedevent->type = which;
    timedevent->interval = delay;
    timedevent->data = data;
    timedevent->timerid = SDL_AddTimer(delay, timercallback, timedevent);
}

/*
 * Handling SDL events.
 */

/* Translate a few key events into commands which all displays have in
 * common.
 */
static command_t handlekeyevent(SDL_Keysym key)
{
    switch (key.sym) {
      case SDLK_F1:     return cmd_showhelp;
      case SDLK_HELP:   return cmd_showhelp;
      case SDLK_ESCAPE: return cmd_quit;
      case SDLK_q:
        return key.mod & KMOD_SHIFT ? cmd_quitprogram : cmd_quit;
    }
    return cmd_none;
}

/* Translate some text events into commands which all displays have in
 * common. In practice, there is only one: the question mark. This
 * needs to be handled as a text event instead of a key event, because
 * keyboards differ in how a question mark is generated.
 */
static command_t handletextevent(char const *text)
{
    int i;

    for (i = 0 ; text[i] ; ++i)
        if (text[i] == '?')
            return cmd_showhelp;
    return cmd_none;
}

/* Identify which button, if any, is under the specified screen
 * coordinates, and update the buttons' hover states appropriately.
 * The return value is -1 if no (active) button exists at this point.
 */
static int sethoverstates(int x, int y)
{
    int which, i;

    which = -1;
    for (i = 0 ; i < buttoncount ; ++i) {
        if (buttons[i]->display != displayed)
            continue;
        if (!buttons[i]->visible || buttons[i]->state == BSTATE_DISABLED)
            continue;
        if (rectcontains(&buttons[i]->pos, x, y)) {
            which = i;
            if (!(buttons[i]->state & BSTATE_HOVER)) {
                buttons[i]->state ^= BSTATE_HOVER;
                updatedisplay = TRUE;
            }
        } else {
            if (buttons[i]->state & BSTATE_HOVER) {
                buttons[i]->state ^= BSTATE_HOVER;
                updatedisplay = TRUE;
            }
        }
    }
    return which;
}

/* Handle all mouse events that interact with one of the buttons. Both
 * clicking and dragging events are dealt with here. If a button is
 * successfully selected (the mouse button changing from up to down
 * and back to up while hovering), then the button's selection state
 * will be toggled and its action callback (if any) will be invoked.
 * The return value is the user command to return to the main program,
 * or cmd_none if no user command was produced.
 */
static command_t handlemouseevent(SDL_Event const *event)
{
    static int mousetrap = -1;
    int n;

    switch (event->type) {
      case SDL_MOUSEBUTTONDOWN:
        if (event->button.button != SDL_BUTTON_LEFT)
            break;
        if (mousetrap >= 0) {
            buttons[mousetrap]->state &= ~BSTATE_DOWN;
            mousetrap = -1;
            updatedisplay = TRUE;
        }
        n = sethoverstates(event->button.x, event->button.y);
        if (n >= 0) {
            mousetrap = n;
            buttons[n]->state |= BSTATE_DOWN;
            updatedisplay = TRUE;
        }
        break;
      case SDL_MOUSEMOTION:
        if (mousetrap < 0) {
            sethoverstates(event->motion.x, event->motion.y);
            break;
        }
        if (rectcontains(&buttons[mousetrap]->pos,
                         event->motion.x, event->motion.y)) {
            if (!(buttons[mousetrap]->state & BSTATE_DOWN)) {
                buttons[mousetrap]->state |= BSTATE_DOWN;
                updatedisplay = TRUE;
            }
        } else {
            if (buttons[mousetrap]->state & BSTATE_DOWN) {
                buttons[mousetrap]->state &= ~BSTATE_DOWN;
                updatedisplay = TRUE;
            }
        }
        break;
      case SDL_MOUSEBUTTONUP:
        if (event->button.button != SDL_BUTTON_LEFT)
            return cmd_none;
        n = sethoverstates(event->button.x, event->button.y);
        if (mousetrap >= 0) {
            updatedisplay = TRUE;
            buttons[mousetrap]->state &= ~BSTATE_DOWN;
            if (mousetrap == n) {
                buttons[n]->state ^= BSTATE_SELECT;
                mousetrap = -1;
                if (buttons[n]->action)
                    (*buttons[n]->action)(buttons[n]->state & BSTATE_SELECT);
                return buttons[n]->cmd;
            }
            mousetrap = -1;
        }
        break;
    }
    return cmd_none;
}

/* Handle a timed event. The return value is either a user command to
 * return to the main program, or cmd_none if the event needs no
 * further attention.
 */
static command_t handletimedevent(int type, timedeventinfo *timedevent)
{
    animinfo *anim;

    if (timedevent->timerid == 0)
        return cmd_none;

    switch (type) {
      case event_none:
        SDL_RemoveTimer(timedevent->timerid);
        timedevent->timerid = 0;
        timedevent->interval = 0;
        break;
      case event_redraw:
        timedevent->type = event_none;
        return cmd_redraw;
      case event_unget:
        timedevent->type = event_none;
        return (command_t)(intptr_t)timedevent->data;
      case event_animate:
        anim = timedevent->data;
        if (anim->steps > 0) {
            if (anim->pval1)
                *anim->pval1 += (anim->destval1 - *anim->pval1) / anim->steps;
            if (anim->pval2)
                *anim->pval2 += (anim->destval2 - *anim->pval2) / anim->steps;
            --anim->steps;
        } else {
            if (anim->callback)
                (*anim->callback)(anim->data, 1);
            anim->pval1 = NULL;
            anim->pval2 = NULL;
            anim->callback = NULL;
            anim->inuse = 0;
            timedevent->type = event_none;
        }
        return cmd_redraw;
    }

    return cmd_none;
}

/* Process one SDL event. Keyboard and mouse events are delegated to
 * the appropriate function. Window events trigger a redraw (and maybe
 * a change in layout).
 */
static command_t handleevent(SDL_Event *event)
{
    SDL_Point size, requestedsize;

    if (event->type == timedeventid)
        return handletimedevent(event->user.code, event->user.data1);

    switch (event->type) {
      case SDL_KEYDOWN:
        return handlekeyevent(event->key.keysym);
      case SDL_TEXTINPUT:
        return handletextevent(event->text.text);
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEMOTION:
        return handlemouseevent(event);
      case SDL_WINDOWEVENT:
        switch (event->window.event) {
          case SDL_WINDOWEVENT_RESIZED:
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            requestedsize.x = event->window.data1;
            requestedsize.y = event->window.data2;
            size = measurelayout(requestedsize);
            if (size.x != requestedsize.x || size.y != requestedsize.y)
                SDL_SetWindowSize(window, size.x, size.y);
            recordwindowsize(size);
            return cmd_redraw;
          case SDL_WINDOWEVENT_EXPOSED:
            return cmd_redraw;
        }
        break;
      case SDL_QUIT:
        return cmd_quitprogram;
    }
    return cmd_none;
}

/*
 * The program's event loop.
 */

/* Forward declaration of a mutually-recursive function.
 */
static int runhelpdisplay(void);

/* Render a user alert (a "ding") by brightening the window and
 * scheduling a redraw to restore the window's normal appearance a
 * moment later.
 */
static void flashdisplay(void)
{
    SDL_BlendMode mode;

    SDL_GetRenderDrawBlendMode(_graph.renderer, &mode);
    SDL_SetRenderDrawBlendMode(_graph.renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(_graph.renderer, 255, 255, 255, 80);
    SDL_RenderFillRect(_graph.renderer, NULL);
    SDL_SetRenderDrawBlendMode(_graph.renderer, mode);
    scheduletimedevent(event_redraw, 128, NULL);
}

/* Draw the display. The basic window appearance is delegated to the
 * appropriate function, depending on which display is currently
 * active. Buttons are rendered atop that.
 */
static void render(void)
{
    int i;

    displays[displayed].render();

    for (i = 0 ; i < buttoncount ; ++i)
        if (buttons[i]->display == displayed && buttons[i]->visible)
            SDL_RenderCopy(_graph.renderer, buttons[i]->texture,
                           &buttons[i]->rects[buttons[i]->state],
                           &buttons[i]->pos);

    if (dinging) {
        dinging = FALSE;
        flashdisplay();
    }
    SDL_RenderPresent(_graph.renderer);
    garbagecollecttextures();
}

/* Automatically translate keypad key events to regular keys, so that
 * they don't need to be handled separately.
 */
static void translatekeysym(SDL_Keysym *keysym)
{
    switch (keysym->sym) {
      case SDLK_KP_8:           keysym->sym = SDLK_UP;          break;
      case SDLK_KP_2:           keysym->sym = SDLK_DOWN;        break;
      case SDLK_KP_4:           keysym->sym = SDLK_LEFT;        break;
      case SDLK_KP_6:           keysym->sym = SDLK_RIGHT;       break;
      case SDLK_KP_ENTER:       keysym->sym = SDLK_RETURN;      break;
    }
}

/* Run the event loop until a command is generated, which is then
 * returned to the caller. The event handler for the current display
 * gets to process each event first. If the display handler doesn't
 * generate a user command, then the event is given to the shared
 * event handler. If this handler also doesn't generate a user
 * command, the function loops. Commands to toggle the help display
 * are handled locally; all other commands are passed back up.
 */
static command_t eventloop(void)
{
    SDL_Event event;
    command_t cmd;

    for (;;) {
        if (updatedisplay) {
            updatedisplay = FALSE;
            render();
        }
        if (!SDL_WaitEvent(&event))
            return err("SDL_WaitEvent");
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
            translatekeysym(&event.key.keysym);
        cmd = displays[displayed].eventhandler(&event);
        if (!cmd)
            cmd = handleevent(&event);
        if (cmd == cmd_redraw) {
            updatedisplay = TRUE;
        } else if (cmd == cmd_showhelp) {
            if (displayed == DISPLAY_HELP)
                return cmd_quit;
            if (!runhelpdisplay())
                return cmd_quitprogram;
            updatedisplay = TRUE;
            cmd = cmd_none;
        }
        if (cmd)
            return cmd;
    }
}

/*
 * Contained event loops for displays other than the main game display.
 */

/* Bring up the options dialog, and run the event loop until the user
 * dismisses it.
 */
static void runoptionsdisplay(settingsinfo *settings)
{
    showoptions(settings, TRUE);
    updatedisplay = TRUE;
    for (;;) {
        switch (eventloop()) {
          case cmd_changesettings:
          case cmd_quit:
          case cmd_quitprogram:
            showoptions(settings, FALSE);
            sdlui_setcardanimationflag(settings->animation);
            updatedisplay = TRUE;
            return;
        }
    }
}

/* Switch to the list display, and run the event loop until the user
 * either selects a game or exits the program. The return value is the
 * ID number of the selected game, or -1 if the user opted to exit.
 */
static int runlistdisplay(int gameid)
{
    initlistselection(gameid);
    displayed = DISPLAY_LIST;
    updatedisplay = TRUE;
    for (;;) {
        switch (eventloop()) {
          case cmd_select:
            resetgamedisplay();
            displayed = DISPLAY_GAME;
            updatedisplay = TRUE;
            return getselection();
          case cmd_quit:
          case cmd_quitprogram:
            return -1;
        }
    }
}

/* Switch to the help display, and run the event loop until the user
 * leaves it. The return value is false if the user indicated a desire
 * to exit the program.
 */
static int runhelpdisplay(void)
{
    int prevdisplayed;

    prevdisplayed = displayed;
    displayed = DISPLAY_HELP;
    updatedisplay = TRUE;
    for (;;) {
        switch (eventloop()) {
          case cmd_showhelp:
          case cmd_quit:
            displayed = prevdisplayed;
            updatedisplay = TRUE;
            return TRUE;
          case cmd_quitprogram:
            return FALSE;
        }
    }
}

/*
 * Internal functions.
 */

/* True if the rectangle contains the given coordinate.
 */
int rectcontains(SDL_Rect const *rect, int x, int y)
{
    return x >= rect->x && x < rect->x + rect->w &&
           y >= rect->y && y < rect->y + rect->h;
}

/* Add a button to one of the displays.
 */
void addbutton(button *btn)
{
    int n;

    n = buttoncount++;
    buttons = reallocate(buttons, buttoncount * sizeof *buttons);
    buttons[n] = btn;
}

/* Set the current color for rendering text.
 */
void settextcolor(SDL_Color color)
{
    currentcolor = color;
}

/* Render a string of text in the given font and color. The align
 * parameter should be positive to left-align the text at the
 * given position, negative to right-align the text, or zero to
 * center the text on the position.
 */
int drawtext(char const *string, int x, int y, int align, TTF_Font *font)
{
    SDL_Surface *image;
    SDL_Texture *texture;
    SDL_Rect rect;

    if (!*string)
        return 0;
    image = TTF_RenderUTF8_Blended(font, string, currentcolor);
    texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    rect.x = x - (align > 0 ? 0 : align < 0 ? image->w : image->w / 2);
    rect.y = y;
    rect.w = image->w;
    rect.h = image->h;
    SDL_FreeSurface(image);
    SDL_RenderCopy(_graph.renderer, texture, NULL, &rect);
    releasetexture(texture);
    return rect.w;
}

/* Numbers get rendered repeatedly in this program, so it seems like a
 * good idea to do a little caching and reuse of the generated
 * textures. Each texture is cached, and if the same number is later
 * displayed using the same color and font, the cached texture will be
 * reused.
 */
int drawnumber(int number, int x, int y, int align, TTF_Font *font)
{
    static numbercacheinfo cache[8];
    static int cachesize = sizeof cache / sizeof *cache;

    SDL_Surface *img;
    SDL_Rect rect;
    char buf[16];
    int free, i;

    free = -1;
    for (i = 0 ; i < cachesize ; ++i) {
        if (number == cache[i].number && font == cache[i].font &&
                    !memcmp(&currentcolor, &cache[i].color, sizeof(SDL_Color)))
            break;
        if (!cache[i].texture)
            free = i;
    }

    if (i == cachesize) {
        if (free >= 0) {
            i = free;
        } else {
            i = cachesize - 1;
            releasetexture(cache[0].texture);
            memmove(cache, cache + 1, i * sizeof *cache);
        }
        sprintf(buf, "%d", number);
        img = TTF_RenderUTF8_Blended(font, buf, currentcolor);
        cache[i].number = number;
        cache[i].color = currentcolor;
        cache[i].font = font;
        cache[i].w = img->w;
        cache[i].h = img->h;
        cache[i].texture = SDL_CreateTextureFromSurface(_graph.renderer, img);
        SDL_FreeSurface(img);
    }

    rect.w = cache[i].w;
    rect.h = cache[i].h;
    rect.x = x - (align > 0 ? 0 : align < 0 ? rect.w : rect.w / 2);
    rect.y = y;
    SDL_RenderCopy(_graph.renderer, cache[i].texture, NULL, &rect);
    return rect.w;
}

/* Return an animinfo object from a static pool. Animations are run by
 * timers, and so it is necessary to keep a pool that allows the
 * object to remain valid for a brief time after its timer has
 * ostensibly been deactivated.
 */
animinfo *getaniminfo(void)
{
    static animinfo pool[8];
    static int lastindex = 0;
    int i;

    i = lastindex;
    for (;;) {
        i = (i + 1) % (sizeof pool / sizeof *pool);
        if (!pool[i].inuse) {
            lastindex = i;
            memset(pool + i, 0, sizeof *pool);
            pool[i].inuse = 1;
            return pool + i;
        }
        if (i == lastindex)
            panic("animinfo pool exhausted");
    }
}

/* Given an initialized animinfo object, start the animation it
 * describes, with msec milliseconds between each step of the
 * animation.
 */
void startanimation(animinfo *anim, int msec)
{
    scheduletimedevent(event_animate, msec, anim);
}

/* End an animation immediately. All pointer fields are cleared out to
 * prevent them from being altered further.
 */
void stopanimation(animinfo *anim, int finish)
{
    anim->steps = 0;
    if (anim->pval1) {
        if (finish)
            *anim->pval1 = anim->destval1;
        anim->pval1 = NULL;
    }
    if (anim->pval2) {
        if (finish)
            *anim->pval2 = anim->destval2;
        anim->pval2 = NULL;
    }
    if (anim->callback) {
        (*anim->callback)(anim->data, finish);
        anim->callback = NULL;
    }
}

/*
 * API functions.
 */

/* Draw the window.
 */
static void sdlui_rendergame(renderparams const *params)
{
    updategamestate(params->gameplay, params->position, params->bookmark);
    render();
}

/* Wait for a command from the user.
 */
static command_t sdlui_getinput(void)
{
    return eventloop();
}

/* Record a command to be injected into the input stream after a delay.
 */
static void sdlui_ungetinput(command_t cmd, int msec)
{
    scheduletimedevent(event_unget, msec, (void*)(intptr_t)cmd);
}

/* Flash the display on the next redraw.
 */
static void sdlui_ding(void)
{
    dinging = TRUE;
}

/* Run the options display.
 */
static int sdlui_changesettings(settingsinfo *settings)
{
    runoptionsdisplay(settings);
    return TRUE;
}

/* Run the list display and return the user's selection. Within this
 * function, the help section on using the list display is added.
 */
static int sdlui_selectgame(int gameid)
{
    char buf[32];

    SDL_SetWindowTitle(window, "Brain Jam");
    sdlui_addhelpsection(listcommandshelptitle, listcommandshelptext, TRUE);
    gameid = runlistdisplay(gameid);
    sdlui_addhelpsection(listcommandshelptitle, NULL, TRUE);
    if (gameid >= 0) {
        sprintf(buf, "Brain Jam - game %04d", gameid);
        SDL_SetWindowTitle(window, buf);
    }
    return gameid;
}

/*
 * External function.
 */

/* Initialize the SDL graphical user interface and return the UI map.
 */
uimap sdlui_initializeui(void)
{
    uimap ui;

    if (!startup() || !createdisplays()) {
        ui.rendergame = NULL;
        return ui;
    }

    finalizeimages();
    sdlui_addhelpsection(redocommandshelptitle, redocommandshelptext, TRUE);
    sdlui_addhelpsection(gamecommandshelptitle, gamecommandshelptext, TRUE);
    sdlui_addhelpsection(gameplayhelptitle, gameplayhelptext, TRUE);

    ui.rendergame = sdlui_rendergame;
    ui.getinput = sdlui_getinput;
    ui.ungetinput = sdlui_ungetinput;
    ui.setshowkeyguidesflag = sdlui_setshowkeyguidesflag;
    ui.setcardanimationflag = sdlui_setcardanimationflag;
    ui.ding = sdlui_ding;
    ui.showwriteindicator = sdlui_showwriteindicator;
    ui.movecard = sdlui_movecard;
    ui.changesettings = sdlui_changesettings;
    ui.selectgame = sdlui_selectgame;
    ui.addhelpsection = sdlui_addhelpsection;
    return ui;
}
