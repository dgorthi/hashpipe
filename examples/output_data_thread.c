/* This thread reads the data written to the buffer by the process thread 
and prints it on the screen. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include "hashpipe.h"
#include "databuf.h"

void *output_thread_run(hashpipe_thread_args_t *args){
    hashpipe_databuf_t *idb = (hashpipe_databuf_t *)args->ibuf;
    printf("Output(): Buffer shmid: %d\n",idb->shmid);
    hashpipe_status_t st = args->st;
    const char *status_key = args->thread_desc->skey;

    int block_idx = 0;
    uint64_t mcnt = 0;
    int rv,sum,offset;

    while(run_threads()){
        hashpipe_status_lock_safe(&st);
        hputi4(st.buf, "OUTBLKIN", block_idx);
        hputs(st.buf, status_key, "waiting");
        hputi8(st.buf, "OUTMCNT", mcnt);
        hashpipe_status_unlock_safe(&st);
        sleep(1);

        /* Wait for sum to be updated */
        while((rv=hashpipe_databuf_wait_filled(idb,block_idx))!=HASHPIPE_OK){
            if(rv==HASHPIPE_TIMEOUT){
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "blocked");
                hashpipe_status_unlock_safe(&st);
                continue;
            }else{
                hashpipe_error("output_thread_run()", 
                               "error waiting for filled databuf");
                pthread_exit(NULL);
                break;
            }
        }

        /*Got new data! Print to screen*/
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "printing");
        hashpipe_status_unlock_safe(&st);
    
        char *abuf = (char *)(idb+sizeof(hashpipe_databuf_t));
        offset = block_idx * idb->block_size;
        memcpy(&sum, abuf+offset, sizeof(int));
        printf("Data in shared memory: %d\n",sum);
        sleep(1);

        hashpipe_databuf_set_free(idb, block_idx);
        block_idx = (block_idx+1)%idb->n_block;
        mcnt++;
    }
    return NULL;
}

hashpipe_thread_desc_t output_data_thread = {
    name: "output_data_thread",
    skey: "OUTSTAT",
    init: NULL,
    run: output_thread_run,
    ibuf_desc: {databuf_create},
    obuf_desc: {NULL}
};

static __attribute__((constructor)) void ctor(){
    register_hashpipe_thread(&output_data_thread);
}
