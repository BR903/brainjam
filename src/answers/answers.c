/* answers/answers.c: managing the user's answers.
 */

#include <stdlib.h>
#include <string.h>
#include "./gen.h"
#include "./types.h"
#include "files/files.h"
#include "answers/answers.h"

/* The array of answers.
 */
static answerinfo *answers = NULL;
static int answercount = 0;

/* Callback for comparing two answerinfo objects.
 */
static int answerinfocmp(void const *a, void const *b)
{
    return ((answerinfo const*)a)->id - ((answerinfo const*)b)->id;
}

/* Look up an answer. NULL is returned if the given game does not
 * have an answer recorded.
 */
static answerinfo *getanswer(int id)
{
    answerinfo key;

    key.id = id;
    return bsearch(&key, answers, answercount, sizeof *answers,
                   answerinfocmp);
}

/* Introduce a new answer to the list of answers. This function
 * assumes that the caller has already verified that no answer
 * already exists for this game.
 */
static answerinfo *addanswer(int id)
{
    answerinfo *answer;

    answers = reallocate(answers, (answercount + 1) * sizeof *answers);
    answer = answers + answercount;
    ++answercount;
    answer->id = id;
    answer->size = 0;
    answer->text = NULL;
    qsort(answers, answercount, sizeof *answers, answerinfocmp);
    return getanswer(id);
}

/* Free all memory associated with the answers array.
 */
static void deallocateanswers(void)
{
    int i;

    for (i = 0 ; i < answercount ; ++i)
        if (answers[i].text)
            deallocate(answers[i].text);
    deallocate(answers);
    answers = NULL;
    answercount = 0;
}

/*
 * External functions.
 */

/* Load the saved answers at the start of the program.
 */
int initializeanswers(void)
{
    int n;

    if (!answercount) {
        n = loadanswerfile(&answers);
        if (n > 0) {
            answercount = n;
            atexit(deallocateanswers);
        }
    }
    return answercount;
}

/* Return the number of answers currently stored.
 */
int getanswercount(void)
{
    return answercount;
}

/* Return the answer for a given game, or NULL if no answer has been
 * recorded.
 */
answerinfo const *getanswerfor(int id)
{
    return getanswer(id);
}

/* Return the answer for a given game. If no answer exists for that
 * game, return the answer immediately preceding it numerically (or
 * the closest possible answer if there are none that come before it).
 * Return NULL only if no answers currently exist.
 */
answerinfo const *getnearestanswer(int id)
{
    int i;

    for (i = 0 ; i < answercount && answers[i].id <= id ; ++i) ;
    return i ? &answers[i - 1] : answers;
}

/* Given an answer, return the first answer with a higher game ID.
 * Return NULL if this answer is the highest.
 */
answerinfo const *getnextanswer(answerinfo const *answer)
{
    if (!answer)
        return NULL;
    ++answer;
    if (answer == answers + answercount)
        return NULL;
    return answer;
}

/* Add an answer to the list and update the answer file. If an answer
 * already exists for this game, it is replaced.
 */
int saveanswer(int gameid, char const *text)
{
    answerinfo *answer;
    int size;

    size = strlen(text);
    answer = getanswer(gameid);
    if (!answer)
        answer = addanswer(gameid);
    if (answer->size > 0 && answer->size < size)
        warn("warning: replacing answer of size %d with one of size %d!",
             answer->size, size);
    if (answer->text)
        deallocate(answer->text);
    answer->text = strallocate(text);
    answer->size = size;
    return saveanswerfile(answers, answercount);
}
