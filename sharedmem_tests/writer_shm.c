#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>

int main(){
    char idata[1024]; //To store input data

    /*ftok(): is used to generate a unique key*/
    key_t key = ftok("shmfile", 65);

    /* shmget(): returns an identifier for 
    * the shared memory segment
    * int shmget(key_t key, size_t size, int shmflg)
    */
    int shmid = shmget(key, 1024, 0666|IPC_CREAT);

    /* shmat to attach to the shared memory.
     * void *shmat(int shmid, void *shmaddr, int shmflg)
    */
    char *str = (char *)shmat(shmid,(void *)0, 0);

    printf("Input Data: ");
    fgets(idata,1024,stdin);

    memcpy(str, idata, 1024);

    printf("Data in shared memory: %s\n",str);

    /* Detach from shared memory */
    shmdt(str);

    return 0;
}
