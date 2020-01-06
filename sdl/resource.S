/* sdl/resource.S: the image data.
 *
 * Images are stored as bitmaps that have been gzipped. This rather
 * unusual choice is made because the bitmap is the only graphical
 * format that SDL handles natively. Zipping them up allows them to
 * be stored in a size that is comparable to PNGs.
 */

#include "resource.h"

/* The playing card image.
 */
.align 16
gzcardset:
.incbin "sdl/cardset.bmp.gz"
gzcardset_end:

/* The sprite sheet image.
 */
.align 16
gzimages:
.incbin "sdl/images.bmp.gz"
gzimages_end:

/* The banner image.
 */
.align 16
gzbanner:
.incbin "sdl/banner.bmp.gz"
gzbanner_end:
