/* files/sessionio.c: reading and writing the session files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "./gen.h"
#include "redo/redo.h"
#include "game/game.h"
#include "files/files.h"
#include "internal.h"

/* Special byte values used in the session files.
 */
#define START_BRANCH    0x20    /* begin a sequence of sibling branches */
#define SIBLING_BRANCH  0x40    /* mark the separation between branches */
#define CLOSE_BRANCH    0x60    /* end a sequence of sibling branches */

/* A byte in a session file is either one of the special values listed
 * above, or it is a combination of a move value plus a bit indicating
 * the value of the better flag.
 */
#define BETTER_FLAG     0x80
#define MOVE_MASK       0x1F
#define movevalue(branch) \
    (((branch)->move & MOVE_MASK) | ((branch)->p->better ? BETTER_FLAG : 0))

/* The name of the current session file.
 */
static char *sessionfilename = NULL;

/* Values needed by the two recursive functions. These values do not
 * change during a recursive sequence of calls, so they are stored
 * here instead of being passed down as arguments and filling up the
 * stack frames.
 */
static FILE *fp;                /* the file being read or written */
static gameplayinfo *gameplay;  /* the game state buffer */
static redo_session *session;   /* the redo session */

/* Forward declaration of a doubly-recursive function.
 */
static void savesession_branchrecurse(redo_branch const *branch);

/* Read a subtree's worth of moves from the session file. For each
 * move, the game state is recreated and added to the session at the
 * given position.
 */
static int loadsession_recurse(redo_position *position)
{
    int moveindex, byte;

    for (;;) {
        byte = fgetc(fp);
        if (byte == EOF || byte == CLOSE_BRANCH)
            return FALSE;
        if (byte == SIBLING_BRANCH)
            return TRUE;
        if (byte == START_BRANCH) {
            while (loadsession_recurse(position))
                restoresavedstate(gameplay, position);
            continue;
        }
        moveindex = byte & MOVE_MASK;
        if (!applymove(gameplay, indextomovecmd(moveindex))) {
            warn("%s:%ld: unable to reinstantiate session tree",
                 sessionfilename, ftell(fp));
            continue;
        }
        position = redo_addposition(session, position, moveindex,
                        &gameplay->state, gameplay->endpoint,
                        byte & BETTER_FLAG ? redo_checklater : redo_nocheck);
    }
}

/* Write a subtree's worth of moves to the session file, rooted at the
 * given position.
 */
static void savesession_recurse(redo_position const *position)
{
    while (position->nextcount == 1) {
        fputc(movevalue(position->next), fp);
        position = position->next->p;
    }
    if (position->nextcount > 0) {
        fputc(START_BRANCH, fp);
        savesession_branchrecurse(position->next);
        fputc(CLOSE_BRANCH, fp);
    }
}

/* Recursively write a set of sibling branches to the session file (in
 * reverse order, so that their current order will be restored when
 * the file is read back in).
 */
static void savesession_branchrecurse(redo_branch const *branch)
{
    if (branch->cdr) {
        savesession_branchrecurse(branch->cdr);
        fputc(SIBLING_BRANCH, fp);
    }
    fputc(movevalue(branch), fp);
    savesession_recurse(branch->p);
}

/*
 * External functions.
 */

/* Change the name of the session file. This function allows the
 * filename to be chosen by code that is distant from the code that
 * calls loadsession() and savesession().
 */
void setsessionfilename(char const *filename)
{
    if (sessionfilename)
        deallocate(sessionfilename);
    sessionfilename = mkpath(filename);
}

/* Read the game tree stored in the session file and recreate it,
 * storing all the positions in the redo session. The game state is
 * restored to the starting position upon return.
 */
int loadsession(redo_session *session_arg, gameplayinfo *gameplay_arg)
{
    if (!sessionfilename)
        return FALSE;

    fp = fopen(sessionfilename, "rb");
    if (!fp) {
        if (errno == ENOENT) {
            return TRUE;
        } else {
            perror(sessionfilename);
            return FALSE;
        }
    }
    gameplay = gameplay_arg;
    session = session_arg;
    loadsession_recurse(redo_getfirstposition(session));
    fclose(fp);
    redo_setbetterfields(session);
    restoresavedstate(gameplay, redo_getfirstposition(session));
    return TRUE;
}

/* Write the moves in the redo session out to the session file.
 */
int savesession(redo_session const *session)
{
    if (!sessionfilename || getreadonly())
        return FALSE;

    fp = fopen(sessionfilename, "wb");
    if (!fp) {
        perror(sessionfilename);
        return FALSE;
    }
    savesession_recurse(redo_getfirstposition(session));
    fclose(fp);
    return TRUE;
}
