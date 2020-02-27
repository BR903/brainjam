/* game/play.c: managing game state in response to user commands.
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "./gen.h"
#include "./types.h"
#include "./decls.h"
#include "./commands.h"
#include "./settings.h"
#include "./ui.h"
#include "redo/redo.h"
#include "solutions/solutions.h"
#include "game/game.h"
#include "internal.h"

/* An entry in a stack of position pointers.
 */
typedef struct stackentry stackentry;
struct stackentry {
    redo_position *position;    /* the value stored on the stack */
    stackentry *next;           /* the next entry in the stack */
};

/* A list of values to pass to handlemove_callback().
 */
typedef struct handlemoveparams {
    gameplayinfo *gameplay;     /* the current game state */
    redo_session *session;      /* the redo session */
    moveinfo move;              /* the move currently being made */
} handlemoveparams;

/* How long to wait before making an automatic move (in milliseconds).
 */
static int const autoplaydelay = 100;

/* True if auto-play on foundations is enabled.
 */
static int autoplay = TRUE;

/* True if animation of card movements is enabled.
 */
static int animation = TRUE;

/* True if branching-redo mode is enabled.
 */
static int branchingredo = TRUE;

/* The position currently being displayed.
 */
static redo_position *currentposition = NULL;

/* The position most recently visited before the current one.
 */
static redo_position *backone = NULL;

/* The stack of bookmarked game states.
 */
static stackentry *positionstack = NULL;

/* The queue of buffered user commands.
 */
static command_t commandbuffer[6];
static int bufferedcommandcount = 0;

/*
 * The bookmark stack.
 */

/* Return true if the bookmark stack is currently empty.
 */
static int isstackempty(void)
{
    return positionstack == NULL;
}

/* Bookmark a position.
 */
static void stackpush(redo_position *position)
{
    stackentry *entry;

    entry = allocate(sizeof *entry);
    entry->position = position;
    entry->next = positionstack;
    positionstack = entry;
}

/* Return the most recently bookmarked position.
 */
static redo_position *stackpop(void)
{
    redo_position *pos;
    stackentry *entry;

    if (isstackempty())
        return NULL;
    entry = positionstack;
    pos = entry->position;
    positionstack = entry->next;
    deallocate(entry);
    return pos;
}

/* Walk the stack and delete any bookmark for a position.
 */
static void stackdelete(redo_position const *position)
{
    stackentry *entry, *prev;

    entry = positionstack;
    prev = NULL;
    while (entry) {
        if (entry->position == position) {
            if (prev) {
                prev->next = entry->next;
                deallocate(entry);
                entry = prev->next;
            } else {
                positionstack = entry->next;
                deallocate(entry);
                entry = positionstack;
            }
        }
        if (!entry)
            break;
        prev = entry;
        entry = entry->next;
    }
}

/*
 * Providing a queue of commands.
 */

/* Store a command on the queue of buffered commands. The return value
 * is false if the queue is already full. If the queue is almost full,
 * the command will be ignored if it is a repetition of the last
 * command.
 */
static int buffercommand(command_t cmd)
{
    int const commandbuffersize = sizeof commandbuffer / sizeof *commandbuffer;

    if (bufferedcommandcount >= commandbuffersize)
        return FALSE;
    if (bufferedcommandcount >= commandbuffersize - 2)
        if (cmd == commandbuffer[bufferedcommandcount - 1])
            return FALSE;
    commandbuffer[bufferedcommandcount] = cmd;
    ++bufferedcommandcount;
    return TRUE;
}

/* Return true if the command buffer is not empty.
 */
static int commandsinbuffer(void)
{
    return bufferedcommandcount > 0;
}

/* Remove and return the oldest command in the command buffer. Return
 * zero if the buffer is empty.
 */
static command_t unbuffercommand(void)
{
    command_t cmd;

    if (bufferedcommandcount <= 0)
        return cmd_none;
    cmd = commandbuffer[0];
    --bufferedcommandcount;
    memmove(commandbuffer, commandbuffer + 1,
            bufferedcommandcount * sizeof *commandbuffer);
    return cmd;
}

/*
 * How cards are moved.
 */

/* Look for a card that can be moved directly onto a foundation pile.
 * If one is found, the return value is the move command for that
 * move. Zero is returned if no such move is currently available. (If
 * there is more than one card available, the function will prefer a
 * card with the same suit as that of the previous such move.)
 */
static movecmd_t findfoundationmove(gameplayinfo const *gameplay)
{
    static int lastsuit = 0;
    place_t from, to;
    card_t card;
    int ret, suit, retsuit;

    ret = 0;
    for (from = MOVEABLE_PLACE_1ST ; from < MOVEABLE_PLACE_END ; ++from) {
        card = gameplay->inplay[from];
        if (!isemptycard(card)) {
            suit = card_suit(card);
            to = foundationplace(suit);
            if (gameplay->inplay[to] + RANK_INCR == card) {
                ret = placetomovecmd1(from);
                retsuit = suit;
                if (suit == lastsuit)
                    return ret;
            }
        }
    }
    if (ret)
        lastsuit = retsuit;
    return ret;
}

/* Delete undone moves branching forward from the given position
 * recursively. This function provides the behavior of a simple
 * undo-redo system, by discarding an undone branch when a new branch
 * is explored.
 */
static redo_position *eraseundonepositions(redo_session *session,
                                           redo_position *position)
{
    if (position->next)
        eraseundonepositions(session, position->next->p);
    stackdelete(position);
    return redo_dropposition(session, position);
}

/* Finish the process of a moving a card, as started by handlemove().
 * Game state is updated, and the move is added to the redo session.
 * If the move created a new and shorter solution, it is saved to
 * disk. Finally, if autoplay is enabled, a scan for foundation moves
 * is scheduled.
 */
static void handlemove_callback(void *data)
{
    handlemoveparams *params;
    gameplayinfo *gameplay;
    redo_session *session;
    redo_position *pos;
    moveinfo move;

    params = data;
    gameplay = params->gameplay;
    session = params->session;
    move = params->move;
    deallocate(params);

    finishmove(gameplay, move);

    backone = currentposition;
    pos = redo_getnextposition(currentposition, movecmdtoindex(move.cmd));
    if (pos) {
        currentposition = pos;
        return;
    }

    if (currentposition->next && !branchingredo)
        eraseundonepositions(session, currentposition->next->p);
    currentposition = redo_addposition(session, currentposition,
                                       movecmdtoindex(move.cmd),
                                       &gameplay->state, gameplay->endpoint,
                                       redo_check);

    pos = redo_getfirstposition(session);
    if (pos->solutionsize != gameplay->bestsolution) {
        if (!gameplay->bestsolution ||
                        gameplay->bestsolution > pos->solutionsize) {
            savesolution(gameplay->configid, session);
            gameplay->bestsolution = pos->solutionsize;
            showsolutionwrite();
        }
    }

    if (commandsinbuffer())
        ungetinput(unbuffercommand(), 0);
    else if (autoplay)
        ungetinput(cmd_autoplay, animation ? 0 : autoplaydelay);
}

/* Accept a move command and cause the move to be initiated. After
 * verifying that the move is valid, and that a previous change is not
 * still in progress, the change to the game state is begun. After any
 * animation has finished, handlemove_callback() will complete the
 * move. (Note that in the case where an earlier move is still in
 * progress but this new move does not intersect with it, this
 * function will still refuse the move, but will schedule it to be
 * automatically retried.)
 */
static int handlemove(gameplayinfo *gameplay, redo_session *session,
                      movecmd_t movecmd)
{
    handlemoveparams *params;
    moveinfo move;
    position_t srcpos, destpos;

    move = findmoveinfo(gameplay, movecmd);
    if (!move.cmd)
        return FALSE;
    if (gameplay->locked & ((1 << move.from) | (1 << move.to)))
        return FALSE;
    if (gameplay->locked) {
        buffercommand(movecmd);
        return FALSE;
    }

    srcpos = gameplay->state[cardtoindex(move.card)];
    destpos = placetopos(move.to);
    if (istableaupos(destpos))
        destpos += gameplay->depth[move.to];

    beginmove(gameplay, move);

    params = allocate(sizeof *params);
    params->gameplay = gameplay;
    params->session = session;
    params->move = move;
    movecard(move.card, srcpos, destpos, handlemove_callback, params);

    return TRUE;
}

/*
 * State navigation.
 */

/* Update the current position. The existing current position is
 * stashed in backone, and the new position's state is loaded into the
 * current game. This function provides the last step of handling most
 * of the user commands involving state navigation.
 */
static void moveposition(gameplayinfo *gameplay, redo_position *pos)
{
    if (!pos) {
        ding();
        return;
    }
    backone = currentposition;
    currentposition = pos;
    restoresavedstate(gameplay, pos);
}

/* Set all mru moves forward from this position to those of the
 * shortest solution.
 */
static void setminimalpath(redo_position *pos)
{
    redo_branch *branch;

    while (pos && pos->solutionsize) {
        for (branch = pos->next ; branch ; branch = branch->cdr)
            if (branch->p->solutionsize == pos->solutionsize)
                break;
        if (!branch)
            break;
        pos = redo_getnextposition(pos, branch->move);
    }
}

/* Interpret and apply a single navigation command. Nearly all of the
 * keys that don't map to move commands are handled in this switch
 * statement. The function return false if the user asked to quit the
 * current game. (Note that many of these commands are unavailable if
 * the branching redo feature is disabled.)
 */
static int handlenavkey(gameplayinfo *gameplay, redo_session *session, int cmd)
{
    redo_position *pos;
    int i;

    if (!branchingredo) {
        switch (cmd) {
          case cmd_undo10:              cmd = cmd_nop;          break;
          case cmd_redo10:              cmd = cmd_nop;          break;
          case cmd_undotobranch:        cmd = cmd_nop;          break;
          case cmd_redotobranch:        cmd = cmd_nop;          break;
          case cmd_switchtobetter:      cmd = cmd_nop;          break;
          case cmd_pushbookmark:        cmd = cmd_nop;          break;
          case cmd_popbookmark:         cmd = cmd_nop;          break;
          case cmd_swapbookmark:        cmd = cmd_nop;          break;
          case cmd_dropbookmark:        cmd = cmd_nop;          break;
          case cmd_setminimalpath:      cmd = cmd_nop;          break;
        }
    }

    switch (cmd) {
      case cmd_erase:
        pos = redo_dropposition(session, currentposition);
        if (pos != currentposition) {
            stackdelete(currentposition);
            currentposition = NULL;
            moveposition(gameplay, pos);
        } else {
            ding();
        }
        break;
      case cmd_jumptostart:
        moveposition(gameplay, redo_getfirstposition(session));
        break;
      case cmd_jumptoend:
        for (pos = currentposition ; pos->next ; pos = pos->next->p) ;
        moveposition(gameplay, pos);
        break;
      case cmd_undo:
        moveposition(gameplay, currentposition->prev);
        break;
      case cmd_redo:
        if (currentposition->next)
            handlemove(gameplay, session,
                       indextomovecmd(currentposition->next->move));
        break;
      case cmd_undo10:
        pos = currentposition;
        for (i = 0 ; i < 10 && pos->prev ; ++i)
            pos = pos->prev;
        if (pos != currentposition)
            moveposition(gameplay, pos);
        break;
      case cmd_redo10:
        pos = currentposition;
        for (i = 0 ; i < 10 && pos->next ; ++i)
            pos = pos->next->p;
        if (pos != currentposition)
            moveposition(gameplay, pos);
        break;
      case cmd_undotobranch:
        if (!currentposition->prev) {
            ding();
            break;
        }
        pos = currentposition;
        while (pos->prev) {
            pos = pos->prev;
            if (pos->nextcount > 1)
                break;
        }
        moveposition(gameplay, pos);
        break;
      case cmd_redotobranch:
        if (!currentposition->next) {
            ding();
            break;
        }
        pos = currentposition;
        while (pos->next) {
            pos = pos->next->p;
            if (pos->nextcount > 1)
                break;
        }
        moveposition(gameplay, pos);
        break;
      case cmd_switchtobetter:
        if (!currentposition->better) {
            ding();
            break;
        }
        pos = currentposition;
        while (pos->better)
            pos = pos->better;
        moveposition(gameplay, pos);
        break;
      case cmd_switchtoprevious:
        moveposition(gameplay, backone);
        break;
      case cmd_pushbookmark:
        stackpush(currentposition);
        break;
      case cmd_popbookmark:
        if (!isstackempty())
            moveposition(gameplay, stackpop());
        break;
      case cmd_swapbookmark:
        if (!isstackempty()) {
            pos = stackpop();
            stackpush(currentposition);
            moveposition(gameplay, pos);
        }
        break;
      case cmd_dropbookmark:
        if (isstackempty())
            ding();
        else
            stackpop();
        break;
      case cmd_setminimalpath:
        setminimalpath(currentposition);
        break;
      case cmd_changesettings:
        if (changesettings(getcurrentsettings()))
            applysettings(TRUE);
        break;
      case cmd_quit:
        while (stackpop()) ;
        return FALSE;
    }
    return TRUE;
}

/*
 * External functions.
 */

/* Enable or disable autoplay.
 */
void setautoplay(int f)
{
    autoplay = f;
}

/* Enable or disable animation of card moves.
 */
void setanimation(int f)
{
    animation = setcardanimationflag(f);
}

/* Enable or disable branching redo.
 */
void setbranching(int f)
{
    branchingredo = f;
}

/* Enter the central loop of game play. Render the game state, wait
 * for a command to be input, apply it to the game state and the redo
 * session, and loop. Continue until one of the quit commands is
 * received.
 */
int gameplayloop(gameplayinfo *gameplay, redo_session *session)
{
    renderparams params;
    int ch;

    currentposition = redo_getfirstposition(session);
    gameplay->bestsolution = currentposition->solutionsize;
    gameplay->locked = 0;
    backone = currentposition;

    for (;;) {
        params.gameplay = gameplay;
        params.position = currentposition;
        params.bookmark = !isstackempty();
        rendergame(&params);
        ch = getinput();
        if (ch == cmd_quitprogram)
            return FALSE;
        if (ch == cmd_autoplay)
            ch = findfoundationmove(gameplay);
        if (ismovecmd(ch)) {
            if (!gameplay->moveable) {
                ding();
                continue;
            }
            if (!handlemove(gameplay, session, ch))
                ding();
            continue;
        } else if (ch) {
            if (!handlenavkey(gameplay, session, ch))
                return TRUE;
        }
    }
}
