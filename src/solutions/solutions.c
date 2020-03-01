/* solutions/solutions.c: managing the user's solutions.
 */

#include <stdlib.h>
#include <string.h>
#include "./gen.h"
#include "./types.h"
#include "files/files.h"
#include "game/game.h"
#include "redo/redo.h"
#include "solutions/solutions.h"

/* The array of solutions.
 */
static solutioninfo *solutions = NULL;
static int solutioncount = 0;

/* Callback for comparing two solutioninfo objects.
 */
static int solutioninfocmp(void const *a, void const *b)
{
    return ((solutioninfo const*)a)->id - ((solutioninfo const*)b)->id;
}

/* Look up a solution. NULL is returned if the given game does not
 * have a solution recorded.
 */
static solutioninfo *getsolution(int id)
{
    solutioninfo key;

    key.id = id;
    return bsearch(&key, solutions, solutioncount, sizeof *solutions,
                   solutioninfocmp);
}

/* Introduce a new solution to the list of solutions. This function
 * assumes that the caller has already verified that no solution
 * already exists for this game.
 */
static solutioninfo *addsolution(int id)
{
    solutioninfo *solution;

    solutions = reallocate(solutions, (solutioncount + 1) * sizeof *solutions);
    solution = solutions + solutioncount;
    ++solutioncount;
    solution->id = id;
    solution->size = 0;
    solution->text = NULL;
    qsort(solutions, solutioncount, sizeof *solutions, solutioninfocmp);
    return getsolution(id);
}

/* Free all memory associated with the solutions array.
 */
static void deallocatesolutions(void)
{
    int i;

    for (i = 0 ; i < solutioncount ; ++i)
        if (solutions[i].text)
            deallocate(solutions[i].text);
    deallocate(solutions);
    solutions = NULL;
    solutioncount = 0;
}

/*
 * External functions.
 */

/* Load the saved solutions at the start of the program.
 */
int initializesolutions(void)
{
    int n;

    if (!solutioncount) {
        n = loadsolutionfile(&solutions);
        if (n > 0) {
            solutioncount = n;
            atexit(deallocatesolutions);
        }
    }
    return solutioncount;
}

/* Return the number of solutions currently stored.
 */
int getsolutioncount(void)
{
    return solutioncount;
}

/* Return the solution for a given game, or NULL if no
 * solution has been recorded.
 */
solutioninfo const *getsolutionfor(int id)
{
    return getsolution(id);
}

/* Return the solution for a given game. If no solution exists for
 * that game, return the solution immediately preceding it numerically
 * (or the closest possible solution if there are none that come
 * before it). Return NULL only if no solutions currently exist.
 */
solutioninfo const *getnearestsolution(int id)
{
    int i;

    for (i = 0 ; i < solutioncount && solutions[i].id <= id ; ++i) ;
    return i ? &solutions[i - 1] : solutions;
}

/* Given a solution, return the first solution with a higher
 * game ID. Return NULL if this solution is the highest.
 */
solutioninfo const *getnextsolution(solutioninfo const *solution)
{
    if (!solution)
        return NULL;
    ++solution;
    if (solution == solutions + solutioncount)
        return NULL;
    return solution;
}

/* Add a solution to the list and update the solution file. If a
 * solution already exists for this game, it is replaced.
 */
int savesolution(int gameid, char const *text)
{
    solutioninfo *solution;
    int size;

    size = strlen(text);
    solution = getsolution(gameid);
    if (!solution)
        solution = addsolution(gameid);
    if (solution->size > 0 && solution->size < size)
        warn("warning: replacing solution of size %d with one of size %d!",
             solution->size, size);
    if (solution->text)
        deallocate(solution->text);
    solution->text = strallocate(text);
    solution->size = size;
    return savesolutionfile(solutions, solutioncount);
}

/* Re-enact a solution, recreating the game state for each move and
 * recording the solution in the redo session. The game state is
 * restored to the starting position upon return.
 */
int replaysolution(gameplayinfo *gameplay, redo_session *session)
{
    solutioninfo const *solution;
    redo_position *position;
    int moveid, i, r;

    solution = getsolution(gameplay->gameid);
    if (!solution)
        return FALSE;

    position = redo_getfirstposition(session);
    for (i = 0 ; i < solution->size ; ++i) {
        if (!ismovecmd(solution->text[i])) {
            warn("game %d: move %d: illegal character \"%c\" in solution",
                 gameplay->gameid, i, solution->text[i]);
            break;
        }
        moveid = mkmoveid(gameplay->inplay[movecmdtoplace(solution->text[i])],
                          ismovecmd2(solution->text[i]));
        if (!applymove(gameplay, solution->text[i])) {
            warn("game %d: move %d: unable to apply move \"%c\" in solution",
                 gameplay->gameid, i, solution->text[i]);
            break;
        }
        position = redo_addposition(session, position, moveid,
                                    &gameplay->state, gameplay->endpoint,
                                    redo_nocheck);
    }

    r = gameplay->endpoint;
    if (!r)
        warn("game %04d: saved solution is incomplete", gameplay->gameid);
    restoresavedstate(gameplay, redo_getfirstposition(session));
    return r;
}
