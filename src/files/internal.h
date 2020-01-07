/* files/internal.h: internal functions of the files module.
 */

#ifndef _files_internal_h_
#define _files_internal_h_

/* Copy the given filename to a new buffer. If filename contains a
 * relative path, the value of datadir is used to produce a full
 * pathname. The caller is responsible for freeing the returned
 * buffer.
 */
extern char *mkpath(char const *filename);

/* Return the state of the read-only flag.
 */
extern int getreadonly(void);

#endif
