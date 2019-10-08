# ENV Setup

## cpio
### extract
```
assume the img is .gz format.
mkdir -p core
cp ./img.gz ./core/core.cpio.gz
cd core
gunzip ./core.cpio.gz
cpio -idm < core.cpio
```

### compress
```
find . | cpio -o -H newc | gzip > ./new.cpio
```

### tips - ownership of files
```
1. chown root:root -R .(not recommend, e.g. improper for /home/usr)
2. sudo su; cpio -idm < core.cpio
```

### tips - login as root
```
After compression,
modify /init or /etc/init.d/rcS [1000 -> 0]
```

## debug
```
add option '-gdb tcp::1234' to run.sh
gdb ./bzImage
target remote :1234
add-symbol-file ./test.ko [addr]
```