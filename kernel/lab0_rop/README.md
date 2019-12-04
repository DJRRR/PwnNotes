# Lab0 Preparation
## A. Build kernel

#### 1. Fetch Linux source code
```
git clone git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
mkdir out
cd linux
git checkout ${TARGET_VERSION}
```
> Note: You can take v4.4 as an example.

#### 2. Global Setting and pre-build
Clean the working directory
```
make O=../out mrproper
make O=../out clean
make mrproper
make clean
git stash
git stash clear
```

Set Architecture and compiler
```
# X86
export GCC=${PATH_TO_SELF_COMPILED_GCC_ROOT_DIR}    # or use default gcc
make CC="$GCC/bin/gcc" O=../out defconfig
make CC="$GCC/bin/gcc" O=../out kvmconfig     # optional

# AARCH64
export ARCH=arm64
export CROSS_COMPILE=${PATH_TO_CROSS_ARCH_TOOCHAIN}/bin/aarch64-linux-gnu-
make O=../out defconfig
make O=../out kvmconfig     # optional
```

#### 3. Edit the building configutration and compile

Edit the **.config** file under **out** dir
```
CONFIG_CONFIGFS_FS=y
CONFIG_SECURITYFS=y
CONFIG_DEBUG_INFO=y
CONFIG_FRAME_POINTER=y
CONFIG_KGDB=y
CONFIG_SECURITY_SELINUX=n
CONFIG_DEBUG_RODATA=n
CONFIG_DEBUG_SET_MODULE_RONX=n
```

Finally build the kernel
```
yes '' | make O=../out oldconfig
make O=../out -j55
```

You can get **System.map** and **vmlinux** under **out** dir.
**bzImage** or **Image** is under **out/arch/${ARCH}/boot**.


## B. Build module
One module sample at https://github.com/vnik5287/kernel_rop.git

Here gives a sample Makefile.
```
obj-m += drv.o       
CC=${PATH_TO_SELF_COMPILED_GCC_ROOT_DIR}/bin/gcc    # or use default gcc
ccflags-y += "-g"
ccflags-y += "-O0"

all:
    make -C ${KDIR} M=$(PWD) modules

clean:
    make -C ${KDIR} M=$(PWD) clean
```

>Tips:
`obj-m` specify the module name, and the build module file is **drv.ko**
`M=`    specify the module source code dir
`${KDIR}`   dir where kernel is built, `out` in the former section
Note: the source works at v4.4, if you compile your module with kernel at a high version, use "lxx%" or "pK" instead of "%p" to output the pointer in kernel.


## C. Build Filesystem (ramdisk)

#### 1. Build BusyBox

Fetch source code.
```
cd ~
mkdir -p tmp &amp;&amp; cd tmp
wget https://busybox.net/downloads/busybox-1.31.0.tar.bz2
tar –jvxf busybox-1.31.0.tar.bz2
cd busybox-1.31.0
```

Set architecture and build config.
```
# X86
export GCC=${PATH_TO_SELF_COMPILED_GCC_ROOT_DIR} # or use default gcc
make CC="$GCC/bin/gcc" defconfig

# AARCH64
export ARCH=arm64
export CROSS_COMPILE=${PATH_TO_CROSS_ARCH_TOOCHAIN}/bin/aarch64-linux-gnu-
make defconfig
```

Edit **.config** to build Busybox as a static binary.
```
CONFIG_STATIC=y
```

Build
```
make install
```

Make sure Busybox has been successfully built.
```
./_install/bin/ls
```


#### 2. Make rootfs
There gives a script to set the data of rootfs.
```
#!/bin/sh
mkdir ramdisk 
cd ramdisk
cp ${BUSYBOX_SOURCE_ROOT_DIR}/_install/* . -a
mkdir proc sys mnt/sysroot dev tmp lib/modules etc -p
mknod dev/console c 5 1
mknod dev/null c 1 3

cat > etc/inittab &lt;&lt; EOF
::sysinit:/etc/init.d/rcS   
::askfirst:-/bin/sh    
::restart:/sbin/init
::ctrlaltdel:/sbin/reboot
::shutdown:/bin/umount -a -r
::shutdown:/sbin/swapoff -a
EOF
chmod +x etc/inittab
echo ''>etc/passwd
echo ''>etc/group
mkdir etc/init.d
cat > etc/init.d/rcS &lt;&lt; EOF
#!/bin/sh
mount proc
mount -o remount,rw /
mount -a    
clear                               
echo "Boot already"
EOF
chmod +x etc/init.d/rcS
cat > etc/fstab &lt;&lt; EOF
# /etc/fstab
proc            /proc        proc    defaults          0       0
sysfs           /sys         sysfs   defaults          0       0
devtmpfs        /dev         devtmpfs  defaults          0       0
EOF
cat > init &lt;&lt; EOF
#!/bin/sh
echo "INIT SCRIPT"
mount -t proc none /proc
mount -t sysfs none /sys
mount -t debugfs none /sys/kernel/debug
mount -t devtmpfs devtmpfs /dev
mkdir /tmp
mount -t tmpfs none /tmp
/bin/sh
EOF
chmod +x init
```


#### 3. Pack the rootfs
```
cd ramdisk
find . -print0 | cpio --null -ov --format=newc  > ../initramfs.img
```


## D. Run qemu to boot Linux kernel

```
# x86
qemu-system-x86_64 \
    -kernel ${IMAGE_PATH}/bzImage \
    -append "console=ttyS0 root=/dev/ram rdinit=/sbin/init" \
    -initrd ${RAMDISK_PATH}/initramfs.img \
    -nographic \
    -m 2G \
    -smp 1 \
    -s 

# AARCH64
qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a57 \
    -kernel ${IMAGE_PATH}/Image \
    -append "console=ttyAMA0 root=/dev/ram rdinit=/sbin/init" \
    -initrd ${RAMDISK_PATH}/initramfs.img \
    -nographic \
    -m 2G \
    -smp 1 \
    -s
```

>**Tips**:
`-nographic`  disable graphical output and redirect serial I/Os to console

`-s`          shorthand for `-gdb tcp::1234`, or you can use `-gdb tcp::xxxx` to specify port

`-initrd file`    use 'file' as initial ram disk, and you can use `-hda/-hdb file` to specify a hard disk

`-hda/-hdb file`  use 'file' as IDE hard disk 0/1 image

`-append cmdline` use 'cmdline' as kernel command line

`-append "console="` specify the device console and option, use 

`console=ttyS0` when boot x86 image, `console=ttyAMA0` when boot arm image

`-append "root=/dev/ram"` specify the root file system (ram/sda)

`-append "rdinit=/sbin/init"` specify the init file, if not set default is `/init`

`-append "debug earlyprintk=serial"` print debug info on console

`-net user,hostfwd=[tcp|udp]:[hostip]:[hostport]-[clientip]:[clientport]`

`-net nic`    old way to create a new NIC and connect it to VLAN 'n'

## Q &amp; A
1. Version of gcc used to build the Kernel and Module?
gcc4.8, gcc4.9, gcc5.4, gcc9.10.
Old kernel version is preferred to compiled with old version gcc.
You can either build gcc or `apt install` an old version gcc.
> Suggest Linux-v4.4 with gcc4.8

2. How to a install a module in kernel?
First make sure the `vermagic` is exactly the same as the target kernel version, use the instruction below.
`strings drv.ko| grep vermagic`
Then pack the driver in your ramdisk.
Finally execute `insmod drv.ko` as root to install the module after the kernel is boot and use `lsmod` to check whether it is successfully installed.
    >Better to do the install process in `init` file.