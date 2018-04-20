/* Define the size and structure of the buffers you need. The bare bones 
 * requires the size of the header (>96 bytes), size of a block and 
 * number of blocks (>1). You can define different sizes and structures 
 * for each buffer you need, in principle.
*/
#include "hashpipe_databuf.h"
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>

hashpipe_databuf_t *databuf_create(int instance_id, int databuf_id){
    return hashpipe_databuf_create(instance_id, databuf_id, 128, 8192, 3);
}
