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
#include "hashpipe_databuf.h"

void *net_thread_run(hashpipe_thread_args_t *args){
    hashpipe_databuf_t *idb = (hashpipe_databuf_t *)args->obuf;
    hashpipe_status_t st = args->st;
    const char *status_key = args->thread_desc->skey;

}

hashpipe_thread_desc_t net_thread = {
    name: "net_thread",
    skey: "NETSTAT",
    init: NULL,
    run: net_thread_run,
    ibuf: {NULL},
    obuf: {input_databuf_create} \\defined in databuf.h
};

