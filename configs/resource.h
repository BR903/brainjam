/* configs/resource.h: the configurations data.
 *
 * This header file declares the data object included by resource.S.
 * As such it needs to be included in the both the assembler file and
 * in C code.
 */

#ifndef _configs_resource_h_
#define _configs_resource_h_

/* A macro for declaring symbols in both C and assembler.
 * compiler and the assembler.
 */
#ifdef __ASSEMBLER__
#define EXTERN(type, name, post)  .globl name
#else
#define EXTERN(type, name, post)  extern type name post;
#endif

/* The size in bytes of the data describing a single configuration.
 */
#define SIZE_CONFIG 32

/* The data array describing all of the predefined configurations.
 */
EXTERN(unsigned char const, configurations, [])

/* The total number of configurations.
 */
EXTERN(int const, configuration_count,)

#endif
