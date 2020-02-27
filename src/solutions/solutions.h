/* solutions/solutions.h: managing the user's solutions.
 *
 * The solutions are the complete set of the user's best solutions for
 * each configuration. Since they are a globally accessible resource,
 * this module is responsible for maintaining them and providing
 * access.
 */

#ifndef _solutions_solutions_h_
#define _solutions_solutions_h_

#include "./types.h"
#include "redo/types.h"

/* This struct encodes the user's solution for a configuration. The
 * actual moves are stored as a string of commands.
 */
struct solutioninfo {
    char *text;                 /* the moves of the solution */
    short size;                 /* how many moves in the solution */
    short id;                   /* the number of the configuration */
};

/* Load the user solutions into memory from the solution file. The
 * return value is the number of solutions that are found therein.
 */
extern int initializesolutions(void);

/* Return the current number of solutions.
 */
extern int getsolutioncount(void);

/* Return the solution for the given configuration, or NULL if the
 * user has no solution for that configuration.
 */
extern solutioninfo const *getsolutionfor(int id);

/* Return the solution for the given configuration if one exists, or
 * if not then the solution for a nearby configuration. When possible,
 * the returned solution will be a configuration for a smaller ID. The
 * return value is NULL only if no solutions currently exist.
 */
extern solutioninfo const *getnearestsolution(int id);

/* Given the solution for a configuration, return the solution for the
 * next configuration, i.e. the first solution for a configuration
 * with a larger ID value. The return value is NULL if there is no
 * solution for a higher configuration.
 */
extern solutioninfo const *getnextsolution(solutioninfo const *solution);

/* Return the index of a randomly selected unsolved configuration. If
 * all configurations are solved, return a configuration with a
 * non-minimal solution. (If all configurations have minimal solutions,
 * then any configuration can be selected.)
 */
extern int pickrandomunsolved(void);

/* Find the next unsolved configuration, starting at startpos and
 * searching forward or backward, depending on whether incr is +1 or
 * -1. If all configurations have solutions, then the next
 * configuration with a non-minimal solution is returned instead. (If
 * all configurations have minimal solutions, then startpos is
 * returned.)
 */
extern int findnextunsolved(int startpos, int incr);

/* Run through the solution recorded for the current configuration,
 * storing the moves in the given redo session. The return value is
 * false if the current configuration does not have a recorded
 * solution. Game state is restored to the starting position upon
 * return.
 */
extern int replaysolution(gameplayinfo *gameplay, redo_session *session);

/* Record the given string as a configuration's solution and write it
 * to the solution file. False is returned if the file cannot be
 * updated.
 */
extern int savesolution(int configid, char const *text);

#endif
