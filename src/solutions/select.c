/* solutions/select.c: functions for selecting unsolved configurations.
 */

#include <stdlib.h>
#include <time.h>
#include "./gen.h"
#include "./types.h"
#include "configs/configs.h"
#include "solutions/solutions.h"

/* Macro to declare a loop over all solutions.
 */
#define foreach_solution(s) \
    for ((s) = getnearestsolution(0) ; (s) ; (s) = getnextsolution(s))

/*
 * External functions.
 */

/* Return the index of a randomly selected unsolved configuration. If
 * all configurations are solved, select a configuration with a
 * non-minimal solution. If all configurations have minimal solutions,
 * select any configuration.
 */
int pickrandomunsolved(void)
{
    solutioninfo const *solution;
    int *array;
    int total, size, n;

    total = getconfigurationcount();
    array = allocate(total * sizeof *array);
    size = 0;
    if (getsolutioncount() == total) {
        foreach_solution(solution)
            if (solution->size > bestknownsolutionsize(solution->id))
                array[size++] = solution->id;
    } else {
        solution = getnearestsolution(0);
        for (n = 0 ; n < total ; ++n) {
            if (solution && solution->id == n)
                solution = getnextsolution(solution);
            else
                array[size++] = n;
        }
    }
    if (size <= 0)
        size = total;
    n = (int)((rand() * (double)size) / (RAND_MAX + 1.0));
    n = array[n];
    deallocate(array);
    return n;
}

/* Starting at startpos and moving either forward or backward,
 * depending on incr, find the next unsolved configuration. If the
 * user already a solution for every configuration, then the next
 * configuration with a non-minimal solution is returned instead.
 */
int findnextunsolved(int startpos, int incr)
{
    solutioninfo const *solution;
    int total, i;

    total = getconfigurationcount();
    if (getsolutioncount() < total) {
        i = startpos;
        for (;;) {
            i += incr;
            if (i < 0)
                i += total;
            if (i >= total)
                i -= total;
            solution = getsolutionfor(i);
            if (!solution || solution->size == 0)
                return i;
            if (i == startpos)
                break;
        }
    } else {
        i = startpos;
        for (;;) {
            i += incr;
            if (i < 0)
                i += total;
            if (i >= total)
                i -= total;
            solution = getsolutionfor(i);
            if (solution->size > bestknownsolutionsize(i))
                return i;
            if (i == startpos)
                break;
        }
    }
    return startpos;
}