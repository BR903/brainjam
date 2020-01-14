/* files/internal.h: internal functions of the files module.
 */

#ifndef _files_internal_h_
#define _files_internal_h_

/* Combine the data directory with the given filename to construct
 * a complete pathname. The caller is responsible for freeing the
 * returned buffer.
 */
extern char *mkdatapath(char const *filename);

/* Combine the settings directory with the given filename to construct
 * a complete pathname. The caller is responsible for freeing the
 * returned buffer.
 */
extern char *mksettingspath(char const *filename);

/* Return the state of the read-only flag.
 */
extern int getreadonly(void);

#endif
