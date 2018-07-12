#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sem.h>

typedef struct {
    char data_type[64];
    int n_block;
    size_t header_size;
    size_t block_size;
    int shmid;
    char *data;
}hashpipe_databuf_t;

hashpipe_databuf_t *hashpipe_databuf_create(int instance_id, int databuf_id, 
                                            size_t header_size, size_t block_size,
                                            int n_block){
    int rv = 0;
    int verify_sizing = 0;
    size_t total_size = header_size + block_size*n_block;

    fprintf(stderr,"Getting shared mem block\n");
    /* Get shared memory block */
    key_t key = ftok("shmfile", 65);
    int shmid = shmget(key + databuf_id - 1, total_size, 0666 | IPC_CREAT);

    fprintf(stderr, "Attaching to shared mem block\n");
    /* Attach */
    hashpipe_databuf_t *d;
    d = shmat(shmid, (void *)0, 0);

    d->header_size = header_size;
    d->block_size = block_size;
    d->n_block = n_block;

    fprintf(stderr, "Verifying sizing parameters... \n");
//    if(verify_sizing) {
//        // Make sure existing sizes match expectaions
//        if(d->header_size != header_size
//        || d->block_size != block_size
//        || d->n_block != n_block) {
//            char msg[256];
//            sprintf(msg, "existing databuf size mismatch "
//                "(%lu + %lu x %d) != (%lu + %ld x %d)",
//                d->header_size, d->block_size, d->n_block,
//                header_size, block_size, n_block);
//            return NULL;
//        }
//    } else {
//      /* Zero out newly created databuf */
//      memset(d, 0, total_size);
//
//      /* Fill params into databuf */
//      d->shmid = shmid;
//      d->header_size = header_size;
//      d->n_block = n_block;
//      d->block_size = block_size;
//      sprintf(d->data_type, "unknown");
//    }

    fprintf(stderr, "Returning.. \n");
    /* Try to lock in memory */
    //rv = shmctl(shmid, SHM_LOCK, NULL);

    return d;
}

int main(){

    hashpipe_databuf_t *db = 
            (hashpipe_databuf_t *) hashpipe_databuf_create(65, 1, 64, 128, 1);

    fprintf(stderr,"Got the databuf\n");
    char buf[1024];
    char *abuf = (char *)(db+sizeof(hashpipe_databuf_t));

    fprintf(stderr,"Write data to shared mem: ");
    fgets(abuf,64,stdin);

    fprintf(stderr,"Data %s\n",abuf);
//    printf("Data: %s",db);
    fprintf(stderr,"Header size: %d\n", db->header_size);
    fprintf(stderr,"Number of blocks: %d\n", db->n_block);

}

