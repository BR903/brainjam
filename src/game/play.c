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
#include "answers/answers.h"
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
 * Note that this delay is only applied when animation is disabled;
 * otherwise it is assumed that the animations already create
 * sufficient delay.
 */
static int const autoplaydelay = 112;

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
static command_t bufferedcommands[6];
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
    int const buffersize = sizeof bufferedcommands / sizeof *bufferedcommands;

    if (bufferedcommandcount >= buffersize)
        return FALSE;
    if (bufferedcommandcount >= buffersize - 2)
        if (cmd == bufferedcommands[bufferedcommandcount - 1])
            return FALSE;
    bufferedcommands[bufferedcommandcount] = cmd;
    ++bufferedcommandcount;
    return TRUE;
}

/* Return true if the command buffer is empty.
 */
static int isbufferempty(void)
{
    return bufferedcommandcount == 0;
}

/* Remove and return the oldest command in the command buffer. Return
 * zero if the buffer is empty.
 */
static command_t unbuffercommand(void)
{
    command_t cmd;

    if (bufferedcommandcount <= 0)
        return cmd_none;
    cmd = bufferedcommands[0];
    --bufferedcommandcount;
    memmove(bufferedcommands, bufferedcommands + 1,
            bufferedcommandcount * sizeof *bufferedcommands);
    return cmd;
}

/*
 * Answers.
 */

/* Return a buffer containing the sequence of moves commands
 * representing the user's current answer. The user is responsible
 * for freeing the returned buffer. The return value is NULL if the
 * answer could not be retrieved.
 */
static char *createanswerstring(gameplayinfo *gameplay,
                                  redo_session *session)
{
    redo_position const *position;
    redo_branch const *branch;
    char *string;
    int failed;
    int size, i;

    position = redo_getfirstposition(session);
    size = position->solutionsize;
    string = allocate(size + 1);
    failed = FALSE;
    for (i = 0 ; i < size ; ++i) {
        for (branch = position->next ; branch ; branch = branch->cdr)
            if (branch->p->solutionsize == size)
                break;
        if (!branch) {
            warn("failed to create answer: no correct move at %d", i + 1);
            failed = TRUE;
            break;
        }
        restoresavedstate(gameplay, position);
        string[i] = moveidtocmd(gameplay, branch->move);
        position = branch->p;
    }
    string[size] = '\0';
    restoresavedstate(gameplay, currentposition);
    if (failed) {
        deallocate(string);
        string = NULL;
    }
    return string;
}

/*
 * Handling move deletion.
 */

/* Delete a game position. In addition to removing it from the session
 * history, the position might also need to be removed from the stack
 * of saved positions, and its removal might require updating the
 * user's best answer size. The return value is the previous
 * position in the history, or NULL if the position could not be
 * deleted.
 */
static redo_position *forgetposition(gameplayinfo *gameplay,
                                     redo_session *session,
                                     redo_position *position)
{
    redo_position *pos;

    pos = redo_dropposition(session, position);
    if (pos == position)
        return NULL;

    gameplay->bestanswersize = redo_getfirstposition(session)->solutionsize;
    stackdelete(position);
    if (currentposition == position)
        currentposition = pos;
    if (backone == position)
        backone = pos;
    return pos;
}

/* Delete undone moves branching forward from the given position
 * recursively. This function allows the redo session to mimic the
 * behavior of a simple undo-redo system, by discarding an undone
 * branch when a new branch is explored.
 */
static redo_position *forgetundonepositions(gameplayinfo *gameplay,
                                            redo_session *session,
                                            redo_position *position)
{
    if (position->next)
        forgetundonepositions(gameplay, session, position->next->p);
    return forgetposition(gameplay, session, position);
}

/*
 * How cards are moved.
 */

/* Look for a card that can be moved directly onto a foundation pile.
 * If one is found, the return value is the move command for that
 * move. Zero is returned if no such move is currently available. If
 * there is more than one card available, the function will prefer a
 * card with the same suit as that of the most recently returned move.
 */
static movecmd_t findfoundationmove(gameplayinfo const *gameplay)
{
    static int lastsuit = 0;
    place_t from, to;
    card_t card;
    int ret, suit, retsuit;

    ret = 0;
    for (from = MOVEABLE_PLACE_1ST ; from < MOVEABLE_PLACE_END ; ++from) {
        card = gameplay->cardat[from];
        if (!isemptycard(card)) {
            suit = card_suit(card);
            to = foundationplace(suit);
            if (gameplay->cardat[to] + RANK_INCR == card) {
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

/* Finish the process of a moving a card, as started by handlemove().
 * Game state is updated, and the move is added to the redo session.
 * If the move created a new and shorter answer, it is saved to
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
    char *buf;
    int moveid;

    params = data;
    gameplay = params->gameplay;
    session = params->session;
    move = params->move;
    deallocate(params);

    finishmove(gameplay, move);
    moveid = mkmoveid(move.card, ismovecmd2(move.cmd));

    backone = currentposition;
    pos = redo_getnextposition(currentposition, moveid);
    if (pos) {
        currentposition = pos;
        return;
    }

    if (currentposition->next && !branchingredo)
        forgetundonepositions(gameplay, session, currentposition->next->p);
    currentposition = recordgamestate(gameplay, session, currentposition,
                                      moveid, redo_check);
    if (currentposition->next)
        updategrafted(gameplay, session, currentposition);

    pos = redo_getfirstposition(session);
    if (pos->solutionsize != gameplay->bestanswersize) {
        if (!gameplay->bestanswersize ||
                        gameplay->bestanswersize > pos->solutionsize) {
            buf = createanswerstring(gameplay, session);
            if (buf) {
                saveanswer(gameplay->gameid, buf);
                deallocate(buf);
                gameplay->bestanswersize = pos->solutionsize;
                showwriteindicator();
            }
        }
    }

    if (!isbufferempty())
        ungetinput(unbuffercommand(), 0);
    else if (autoplay)
        ungetinput(cmd_autoplay, animation ? 0 : autoplaydelay);
}

/* Accept a move command and cause the move to be initiated. After
 * verifying that the move is valid, and that a previous change is not
 * still in progress, the change to the game state is begun. After any
 * animation has finished, handlemove_callback() will complete the
 * move. (Note that in the case where an earlier move is still in
 * progress but this new move does not intersect with it, the move
 * will be stored in the buffer instead, to be resubmitted once the
 * current move is complete.)
 */
static int handlemove(gameplayinfo *gameplay, redo_session *session,
                      movecmd_t movecmd)
{
    handlemoveparams *params;
    moveinfo move;

    move = findmoveinfo(gameplay, movecmd);
    if (!move.cmd)
        return FALSE;
    if (gameplay->locked & ((1 << move.from) | (1 << move.to)))
        return FALSE;
    if (gameplay->locked) {
        buffercommand(movecmd);
        return FALSE;
    }

    beginmove(gameplay, move);

    params = allocate(sizeof *params);
    params->gameplay = gameplay;
    params->session = session;
    params->move = move;
    movecard(gameplay, move.card, move.from, move.to,
             handlemove_callback, params);

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

/* Set the default redo moves forward from this position to those of
 * the shortest answer.
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

/* Replace several navigation commands with other commands (usually
 * nothing) when the branching redo feature is disabled.
 */
static command_t remapcommand(command_t cmd)
{
    if (!branchingredo) {
        switch (cmd) {
          case cmd_undo10:              return cmd_nop;
          case cmd_redo10:              return cmd_nop;
          case cmd_undotobranch:        return cmd_nop;
          case cmd_redotobranch:        return cmd_nop;
          case cmd_jumptostart:         return cmd_nop;
          case cmd_jumptoend:           return cmd_nop;
          case cmd_switchtobetter:      return cmd_nop;
          case cmd_pushbookmark:        return cmd_nop;
          case cmd_popbookmark:         return cmd_nop;
          case cmd_swapbookmark:        return cmd_nop;
          case cmd_dropbookmark:        return cmd_nop;
          case cmd_setminimalpath:      return cmd_nop;
        }
    }
    return cmd;
}

/* Interpret and apply a single navigation command. Nearly all of the
 * keys that don't map to move commands are handled in this switch
 * statement. The function returns false if the user asked to quit the
 * current game.
 */
static int handlenavkey(gameplayinfo *gameplay, redo_session *session,
                        command_t cmd)
{
    redo_position *pos;
    int i;

    switch (remapcommand(cmd)) {
      case cmd_erase:
        pos = forgetposition(gameplay, session, currentposition);
        if (pos)
            moveposition(gameplay, pos);
        else
            ding();
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
                       moveidtocmd(gameplay, currentposition->next->move));
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

/* Run the inner loop of game play. Display the game state, wait for a
 * command to be input, apply it to the game state and the redo
 * session, and loop. Continue until one of the quit commands is
 * received.
 */
int gameplayloop(gameplayinfo *gameplay, redo_session *session)
{
    renderparams params;
    command_t cmd;

    currentposition = redo_getfirstposition(session);
    gameplay->bestanswersize = currentposition->solutionsize;
    gameplay->locked = 0;
    backone = currentposition;

    for (;;) {
        params.gameplay = gameplay;
        params.position = currentposition;
        params.bookmark = !isstackempty();
        rendergame(&params);
        cmd = getinput();
        if (cmd == cmd_quitprogram)
            return FALSE;
        if (cmd == cmd_autoplay)
            cmd = findfoundationmove(gameplay);
        if (ismovecmd(cmd)) {
            if (!gameplay->moveable) {
                ding();
                continue;
            }
            if (!handlemove(gameplay, session, cmd))
                ding();
            continue;
        } else if (cmd) {
            if (!handlenavkey(gameplay, session, cmd))
                return TRUE;
        }
    }
}
