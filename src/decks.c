/* ./decks.c: accessing the decks for each game.
 */

#include <string.h>
#include "./types.h"
#include "./decls.h"
#include "./incbin.h"
#include "./decks.h"

/* The size in bytes of the data for a single game.
 */
#define SIZE_GAMEDATA 32

/* The complete array of games.
 */
INCBIN("gamedata/gamedata.bin", gamedata, gamedata_end);

/* Return the data for a given game ID.
 */
static void const *getgamedata(int id)
{
    return gamedata + id * SIZE_GAMEDATA;
}

/*
 * External functions.
 */

/* Compute the number of games available.
 */
int getdeckcount(void)
{
    return (gamedata_end - gamedata) / SIZE_GAMEDATA;
}

/* The best known solution size is stored in the first seven bits of
 * the game data, biased by the number of cards in a deck.
 */
int bestknownsolutionsize(int id)
{
    return NCARDS + (*(unsigned char const*)getgamedata(id) >> 1);
}

/* Extract the deck order for a game and store it in the given array.
 * The setup data is a bit stream of numerical values. After the first
 * seven bits, each value identifies a card. The values start at six
 * bits in size, large enough to hold a value from 0-51, and gradually
 * diminish as the range of possible values diminishes.
 */
void getgamedeck(card_t cards[NCARDS], int id)
{
    char avail[NCARDS];
    unsigned char const *data;
    int bytepos, bitvalue, size;
    int cardindex, n, i, b;

    data = getgamedata(id);
    memset(avail, 1, sizeof avail);
    bytepos = 0;
    bitvalue = 1;
    size = 6;
    for (i = 0 ; i < NCARDS ; ++i) {
        n = NCARDS - i;
        if ((n & (n - 1)) == 0)
            --size;
        n = 0;
        for (b = 0 ; b < size ; ++b) {
            n <<= 1;
            if (data[bytepos] & bitvalue)
                n |= 1;
            bitvalue >>= 1;
            if (bitvalue == 0) {
                ++bytepos;
                bitvalue = 128;
            }
        }
        for (++n, cardindex = 0 ; (n -= avail[cardindex]) > 0 ; ++cardindex) ;
        cards[i] = indextocard(cardindex);
        avail[cardindex] = 0;
    }
}
