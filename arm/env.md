# ARM Debugging

```
sudo apt-get install gcc-arm-linux-gnueabi
(gcc-arm-linux-gnueabihf & gcc-arm-linux-androideabi)

qemu-arm -g 1234 -L /usr/arm-linux-gnueabi ./arm_bin

sudo apt-get install gdb-multiarch

gdb-multiarch ./arm_bin

gdb> set architecture arm
gdb> target remote :1234

```