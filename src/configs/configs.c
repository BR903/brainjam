/* configs/configs.c: accessing the configurations.
 */

#include <string.h>
#include "./types.h"
#include "./decls.h"
#include "configs/configs.h"
#include "resource.h"

/* Find the setup data for a configuration.
 */
static void const *getconfigurationsetup(int id)
{
    return configurations + id * SIZE_CONFIG;
}

/*
 * External functions.
 */

/* Return the number of configurations.
 */
int getconfigurationcount(void)
{
    return configuration_size / SIZE_CONFIG;
}

/* The best known solution size is stored in the first seven bits of
 * the configuration data, offset by the size of the deck.
 */
int bestknownsolutionsize(int id)
{
    return NCARDS + (*(unsigned char const*)getconfigurationsetup(id) >> 1);
}

/* Extract the deck order for a configuration and store it in the
 * given array. The setup data is a bit stream of numerical values.
 * The values start at six bits in size, large enough to hold a value
 * from 0-51, and gradually diminish as the range of possible values
 * diminishes.
 */
void getdeckforconfiguration(card_t cards[NCARDS], int id)
{
    char avail[NCARDS];
    unsigned char const *data;
    int bytepos, bitvalue, size;
    int cardindex, n, i, b;

    data = getconfigurationsetup(id);
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
