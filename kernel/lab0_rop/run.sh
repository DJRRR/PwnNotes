#!/bin/sh
#qemu-system-x86_64 \
#    -kernel ./bzImage \
#    -append "console=ttyS0 root=/dev/ram rdinit=init" \
#    -initrd ./initramfs.img \
#    -nographic \
#    -m 256M \
#    -smp 1 \

# debug
qemu-system-x86_64 \
    -kernel ./bzImage \
    -append "console=ttyS0 root=/dev/ram" \
    -initrd ./new.cpio \
    -nographic \
    -m 256M \
    -smp 1 \
    -s
