/* This thread takes data written by the recv thread from the input shared 
memory and processes it, writing the output to an output shared memory. */
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

void *process_thread_run(hashpipe_thread_args_t *args){
    hashpipe_databuf_t *idb = (hashpipe_databuf_t *)args->ibuf;
    hashpipe_databuf_t *odb = (hashpipe_databuf_t *)args->obuf;
    hashpipe_status_t st = args->st;
    const char *status_key = args->thread_desc->skey;

    uint64_t mcnt=0;
    int iblk = 0;   // current input block
    int oblk = 0;   // current output block
    int a,b,c;      // temp variables to hold data
    int rv;
    int offset;

    while (run_threads()){
        hashpipe_status_lock_safe(&st);
        hputi4(st.buf, "PROCBLKIN", iblk);
        hputs(st.buf, status_key, "waiting");
        hputi4(st.buf, "PROCBLKOUT", oblk);
        hputi8(st.buf, "PROCMCNT", mcnt);
        hashpipe_status_unlock_safe(&st);
        sleep(1);
        
        /* Wait for iblk to be marked full */
        while((rv=hashpipe_databuf_wait_filled(idb,iblk))!=HASHPIPE_OK){
            if(rv==HASHPIPE_TIMEOUT){
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "process blocked input");
                hashpipe_status_unlock_safe(&st);
                continue;
            }else{
                hashpipe_error("process_thread_run()", 
                               "error waiting for filled databuf");
                pthread_exit(NULL);
                break;
            }
        }

        /*Got new data! Update status and deal with data */
        hashpipe_status_lock_safe(&st);
        hputs(st.buf, status_key, "processing data");
        hashpipe_status_unlock_safe(&st);

        char *ibuf = (char *)(idb+sizeof(hashpipe_databuf_t));
        offset = iblk * idb->block_size;
        memcpy(&a, ibuf+offset, sizeof(int));
        memcpy(&b, ibuf+offset+sizeof(int), sizeof(int));

        printf("Data read: number1: %d \t number2: %d\n",a,b);

        c = a+b;

        /*Mark input block as free*/
        hashpipe_databuf_set_free(idb, iblk);
        iblk = (iblk+1) % idb->n_block;

        /*Wait for output buffer to be free*/
        while((rv=hashpipe_databuf_wait_free(odb, oblk))!=HASHPIPE_OK){
            if (rv==HASHPIPE_TIMEOUT){
                hashpipe_status_lock_safe(&st);
                hputs(st.buf, status_key, "process blocked output");
                hashpipe_status_unlock_safe(&st);
                continue;
            }else{
                hashpipe_error("process_data_thread()", 
                               "error waiting for free out databuf");
                pthread_exit(NULL);
                break;
            }
        }

        /*Write output to buffer*/
        char *obuf = (char *)(odb + sizeof(hashpipe_databuf_t));
        offset = oblk * odb->block_size;
        memcpy(obuf+offset, &c, sizeof(int));

        /*Mark output as filled*/
        hashpipe_databuf_set_filled(odb, oblk);
        oblk = (oblk+1)%odb->n_block;
        mcnt++;

        /*Kill thread if cancelled*/
        pthread_testcancel();
    }
    return THREAD_OK;
}

hashpipe_thread_desc_t process_data_thread = {
    name: "process_data_thread",
    skey: "PROCSTAT",
    init: NULL,
    run: process_thread_run,
    ibuf_desc: {databuf_create},
    obuf_desc: {databuf_create}
};

static __attribute__((constructor)) void ctor(){
    register_hashpipe_thread(&process_data_thread);
}
