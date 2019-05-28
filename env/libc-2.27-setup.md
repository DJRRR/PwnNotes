# Replace libc

## ld.so
```
Given libc-2.27.so

>>> strings libc-2.27.so | grep GNU

>>> GNU C Library (Ubuntu GLIBC 2.27-3ubuntu1) stable release version 2.27.
Compiled by GNU CC version 7.3.0.

>>> https://launchpad.net/ubuntu/+source/glibc/2.27-3ubuntu1

>>> choose arch(amd64)

>>> wget https://launchpad.net/~adconrad/+archive/ubuntu/staging/+build/14768180/+files/libc6_2.27-3ubuntu1_amd64.deb

>>> dpkg -x libc6_2.27-3ubuntu1_amd64.deb ./tmp

>>> cp ./tmp/lib/x86_64-linux-gnu/ld-2.27.so .

>>> modify binary. change the path of ld.so to ./ld-2.27.so

```


## libc.so with debug symbol
```
Given libc-2.27.so

>>> strings libc-2.27.so | grep GNU

>>> GNU C Library (Ubuntu GLIBC 2.27-3ubuntu1) stable release version 2.27.
Compiled by GNU CC version 7.3.0.

>>> https://launchpad.net/ubuntu/+source/glibc/2.27-3ubuntu1

>>> choose arch(amd64)

>>> wget wget https://launchpad.net/~adconrad/+archive/ubuntu/staging/+build/14768180/+files/libc6-dbg_2.27-3ubuntu1_amd64.deb(libc with debug symbol)

>>> dpkg -x ./libc6-dbg_2.27-3ubuntu1_amd64.deb ./dbg

>>> cp ./dbg/usr/lib/debug/lib/x86_64-linux-gnu/libc-2.27.so ./libc-2.27-dbg.so

>>> file libc-2.27-dbg.so

>>> get build id of it:BuildID[sha1]=b417c0ba7cc5cf06d1d1bed6652cedb9253c60d

>>> mv libc-2.27-dbg.so 17c0ba7cc5cf06d1d1bed6652cedb9253c60d.debug(BuildID[2:])

>>> mkdir /usr/lib/debug/.build-id/b4(BuildID[0:2])

>>> cp 17c0ba7cc5cf06d1d1bed6652cedb9253c60d.debug /usr/lib/debug/.build-id/b4

```