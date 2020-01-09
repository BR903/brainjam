/* sdl/resource.h: the image data.
 *
 * This header file declares the data objects included by resource.S.
 * As such it needs to be included in the both the assembler file and
 * in C code.
 */

#ifndef _sdl_resource_h_
#define _sdl_resource_h_

/* A macro for declaring objects in both C and assembler.
 */
#ifdef __ASSEMBLER__
#define EXTERN_PTR(name)  .globl name
#else
#define EXTERN_PTR(name)  extern void const *name;
#endif

/* The program's image sheet.
 */
EXTERN_PTR(gzimages)
EXTERN_PTR(gzimages_end)

/* The deck of cards.
 */
EXTERN_PTR(gzcardset)
EXTERN_PTR(gzcardset_end)

/* The banner images.
 */
EXTERN_PTR(gzbanner)
EXTERN_PTR(gzbanner_end)
EXTERN_PTR(gzheadline)
EXTERN_PTR(gzheadline_end)

#endif
