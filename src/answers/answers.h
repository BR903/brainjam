/* answers/answers.h: managing the user's answers.
 *
 * The answers are the complete set of the user's best answers, or
 * solutions, for each game. Since they are a globally accessible
 * resource, this module is responsible for managing access to them.
 */

#ifndef _answers_answers_h_
#define _answers_answers_h_

#include "./types.h"

/* This struct encapsulates the user's answer for a game. The actual
 * moves are stored as a string of commands.
 */
struct answerinfo {
    char *text;                 /* the moves of the answer */
    int size;                   /* the number of moves in the answer */
    int id;                     /* the ID number of the game */
};

/* Load the user's answers into memory from the answer file. The
 * return value is the number of answers that are found therein.
 */
extern int initializeanswers(void);

/* Return the current number of answers.
 */
extern int getanswercount(void);

/* Return the answer for the given game, or NULL if the user has no
 * answer for that game.
 */
extern answerinfo const *getanswerfor(int id);

/* Return the answer for the given game if one exists, or if not
 * then the answer for a nearby game. When possible, the returned
 * answer will be a game for a smaller ID. The return value is NULL
 * only if no answers currently exist.
 */
extern answerinfo const *getnearestanswer(int id);

/* Given the answer for a game, return the answer for the next game,
 * i.e. the first answer for a game with a larger ID value. The return
 * value is NULL if there is no answer for a higher-numbered game.
 */
extern answerinfo const *getnextanswer(answerinfo const *answer);

/* Return the index of a randomly selected unsolved game. If all games
 * are solved, return a game with a non-minimal answer. (If all games
 * have minimal answers, then any game can be selected.)
 */
extern int pickrandomunsolved(void);

/* Find the next unsolved game, starting at startpos and searching
 * forward or backward, depending on whether incr is +1 or -1. If all
 * games have answers, then the next game with a non-minimal answer is
 * returned instead. (If all games have minimal answers, then startpos
 * is returned.)
 */
extern int findnextunsolved(int startpos, int incr);

/* Record the given string as a game's answer and write it to the
 * answer file. False is returned if the file cannot be updated.
 */
extern int saveanswer(int gameid, char const *text);

#endif
