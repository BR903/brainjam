/* sdl/zrwops.h: reading compressed buffers.
 *
 * This function provides a simple translation layer, providing access
 * to a data buffer that has been deflated using the zlib algorithm
 * (either through the zlib library or directly through gzip). The
 * SDL_RWOps object that it returns allows the compressed buffer to be
 * used with any SDL operation that is normally accomplished with an
 * external data file.
 */

#ifndef _sdl_zrwops_h_
#define _sdl_zrwops_h_

#include <SDL.h>

/* Return a RWops object that inflates a buffer containing data
 * compressed by either zlib or gzip.
 */
extern SDL_RWops *RWFromZStream(void const *input, int size);

/* Macro to encapsulate the loading of a compressed resource. Every
 * resource label has a companion label with an "_end" suffix that
 * marks where the resource data ends, and is used to compute the
 * resource's size.
 */
#define _zsize(name, suffix) ((char const*)&name##suffix - (char const*)&name)
#define getzresource(name) (RWFromZStream(&name, _zsize(name, _end)))

#endif
