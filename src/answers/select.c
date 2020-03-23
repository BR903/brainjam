/* answers/select.c: functions for selecting unsolved games.
 */

#include <stdlib.h>
#include <time.h>
#include "./gen.h"
#include "./types.h"
#include "./decks.h"
#include "answers/answers.h"

/* Macro to declare a loop over all answers.
 */
#define foreach_answer(s)  \
    for ((s) = getnearestanswer(0) ; (s) ; (s) = getnextanswer(s))

/* Return a random integer between 0 and size - 1. Since this is the
 * only piece of code that deals in random values, it takes care of
 * seeding the generator.
 */
static int randomint(int size)
{
    static int seeded = 0;

    if (!seeded) {
        seeded = time(NULL);
        srand(seeded);
    }
    return (int)((rand() * (double)size) / (RAND_MAX + 1.0));
}

/*
 * External functions.
 */

/* Return the ID of a randomly selected unsolved game. If all games
 * are solved, select a game with a non-minimal answer. If all games
 * have minimal answers, select any game.
 */
int pickrandomunsolved(void)
{
    answerinfo const *answer;
    int *array;
    int total, size, n;

    total = getdeckcount();
    array = allocate(total * sizeof *array);
    size = 0;
    if (getanswercount() == total) {
        foreach_answer(answer)
            if (answer->size > bestknownanswersize(answer->id))
                array[size++] = answer->id;
    } else {
        answer = getnearestanswer(0);
        for (n = 0 ; n < total ; ++n) {
            if (answer && answer->id == n)
                answer = getnextanswer(answer);
            else
                array[size++] = n;
        }
    }
    if (size <= 0)
        size = total;
    n = array[randomint(size)];
    deallocate(array);
    return n;
}

/* Starting at startpos and moving either forward or backward,
 * depending on incr, find the next unsolved game. If the user already
 * an answer for every game, then the next game with a non-minimal
 * answer is returned instead.
 */
int findnextunsolved(int startpos, int incr)
{
    answerinfo const *answer;
    int total, i;

    total = getdeckcount();
    if (getanswercount() < total) {
        i = startpos;
        for (;;) {
            i += incr;
            if (i < 0)
                i += total;
            if (i >= total)
                i -= total;
            answer = getanswerfor(i);
            if (!answer || answer->size == 0)
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
            answer = getanswerfor(i);
            if (answer->size > bestknownanswersize(i))
                return i;
            if (i == startpos)
                break;
        }
    }
    return startpos;
}
