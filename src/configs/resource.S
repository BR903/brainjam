/* configs/resource.S: the configurations data.
 */

#include "resource.h"

.align 16

/* The array of configurations.
 */
configurations:
.incbin "configs/configs.bin"

/* The number of configurations stored in the array.
 */
configuration_count:
.int (. - configurations) / SIZE_CONFIG
