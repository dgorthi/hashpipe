/* This is the first thread in the hashpipe plugin. It receives data from a socket
in the most general case, but for the purposes of this tutorial data is taken as 
input from the user. This data is written to a shared memory location. */
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

void *recv_thread_run(hashpipe_thread_args_t *args){
    hashpipe_databuf_t *idb = (hashpipe_databuf_t *)args->obuf;
    hashpipe_status_t st = args->st;
    const char *status_key = args->thread_desc->skey;

    int rv;             // store return of buffer status calls
    int block_idx= 0;   // block being filled
    uint64_t mcnt= 0; 
    int a,b;            // user input numbers
    int offset;         // memory offset for databuf

    while (run_threads()) {    
        /*Update status buffer*/
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "waiting");
        hputi4(st.buf, "NETBKOUT", block_idx);
        hputi8(st.buf, "NETMCNT", mcnt);
        hashpipe_status_unlock_safe(&st);
        sleep(1);
    
        /*Wait for data*/
        while((rv=hashpipe_databuf_wait_free(idb,block_idx))!=HASHPIPE_OK){
            if (rv==HASHPIPE_TIMEOUT){
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "blocked");
                hashpipe_status_unlock_safe(&st);
                continue;
            }else{
                hashpipe_error("recv_thread_run()",
                               "Waiting to free block id %d",block_idx);
                pthread_exit(NULL);
                break;
            }
        }
    
        /* Update status buffer */
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "receiving");
        hashpipe_status_unlock_safe(&st);
    
        /* Storage in db exists beyond the header info-- 
         * you can in principle define your own structure to 
         * arrange the data here.
        */
        char *abuf = (char *)(idb+sizeof(hashpipe_databuf_t));
        
        printf("Input first number: ");
        scanf("%d", &a);
        printf("Second number: ");
        scanf("%d", &b);
        sleep(1);

        offset = block_idx * idb->block_size;    
        memcpy(abuf+offset, &a, sizeof(int));
        memcpy(abuf+offset+sizeof(int), &b, sizeof(int));
    
        /* Set block as full and update block_id */
        hashpipe_databuf_set_filled(idb, block_idx);
        block_idx = (block_idx +1) %idb->n_block;
        mcnt++;
    
        /* Exit if thread has been cancelled */
        pthread_testcancel();
    }

    /* Thread done successfully */
    return THREAD_OK;
}

/* Initialize thread descriptor for this thread */
hashpipe_thread_desc_t recv_data_thread = {
    name: "recv_data_thread",
    skey: "NETSTAT",
    init: NULL,
    run: recv_thread_run,
    ibuf_desc: {NULL},
    obuf_desc: {databuf_create}
};

/* Register to let plugin know */
static __attribute__((constructor)) void ctor(){
    register_hashpipe_thread(&recv_data_thread);
}
