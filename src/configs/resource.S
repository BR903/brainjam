/* configs/resource.S: the configurations data.
 */

#include "resource.h"

.align 16

/* The array of configurations.
 */
configurations:
.incbin "configs/configs.bin"

/* The size of the configurations array in bytes.
 */
configuration_size:
.int . - configurations
