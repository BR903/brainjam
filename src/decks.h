/* ./decks.h: accessing the decks for each game.
 *
 * Each game ID corresponds to a unique initial deck order, which is
 * what makes each game different. Stored with the deck order also is
 * the size of the shortest possible answer for that game.
 */

#ifndef _decks_h_
#define _decks_h_

#include "./types.h"
#include "./decls.h"

/* Return the number of decks available.
 */
extern int getdeckcount(void);

/* Return the size of the best known answer for a deck.
 */
extern int bestknownanswersize(int id);

/* Retrieve the deck of cards for the given ID. The cards are stored
 * in the given array, top card first.
 */
extern void getgamedeck(card_t cards[NCARDS], int id);

#endif
