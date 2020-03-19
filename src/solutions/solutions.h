/* solutions/solutions.h: managing the user's solutions.
 *
 * The solutions are the complete set of the user's best solutions for
 * each game. Since they are a globally accessible resource, this
 * module is responsible for managing access to them.
 */

#ifndef _solutions_solutions_h_
#define _solutions_solutions_h_

#include "./types.h"

/* This struct encodes the user's solution for a game. The actual
 * moves are stored as a string of commands.
 */
struct solutioninfo {
    char *text;                 /* the moves of the solution */
    short size;                 /* how many moves in the solution */
    short id;                   /* the ID number of the game */
};

/* Load the user solutions into memory from the solution file. The
 * return value is the number of solutions that are found therein.
 */
extern int initializesolutions(void);

/* Return the current number of solutions.
 */
extern int getsolutioncount(void);

/* Return the solution for the given game, or NULL if the user has no
 * solution for that game.
 */
extern solutioninfo const *getsolutionfor(int id);

/* Return the solution for the given game if one exists, or if not
 * then the solution for a nearby game. When possible, the returned
 * solution will be a game for a smaller ID. The return value is NULL
 * only if no solutions currently exist.
 */
extern solutioninfo const *getnearestsolution(int id);

/* Given the solution for a game, return the solution for the next
 * game, i.e. the first solution for a game with a larger ID value.
 * The return value is NULL if there is no solution for a higher game.
 */
extern solutioninfo const *getnextsolution(solutioninfo const *solution);

/* Return the index of a randomly selected unsolved game. If all games
 * are solved, return a game with a non-minimal solution. (If all
 * games have minimal solutions, then any game can be selected.)
 */
extern int pickrandomunsolved(void);

/* Find the next unsolved game, starting at startpos and searching
 * forward or backward, depending on whether incr is +1 or -1. If all
 * games have solutions, then the next game with a non-minimal
 * solution is returned instead. (If all games have minimal solutions,
 * then startpos is returned.)
 */
extern int findnextunsolved(int startpos, int incr);

/* Record the given string as a game's solution and write it to the
 * solution file. False is returned if the file cannot be updated.
 */
extern int savesolution(int gameid, char const *text);

#endif
