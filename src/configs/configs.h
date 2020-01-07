/* configs/configs.h: accessing the configurations.
 *
 * The configurations hold the data for the initial deck order, which
 * is what makes each game different. This module is responsible for
 * providing access to this data.
 */

#ifndef _configs_configs_h_
#define _configs_configs_h_

#include "./types.h"
#include "./decls.h"

/* Return the number of configurations available.
 */
extern int getconfigurationcount(void);

/* Return the size of the best known solution for a configuration.
 */
extern int bestknownsolutionsize(int id);

/* Retrieve the deck of cards for the given configuration. The cards
 * are stored in the given array, top card first.
 */
extern void getdeckforconfiguration(card_t cards[NCARDS], int id);

#endif
