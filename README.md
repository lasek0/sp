# sp
Binary struct pack and unpack. Based on python's struct.pack and struct.unpack

```
$ sp
Binary struct pack and unpack.
Based on python's struct.pack and struct.unpack
Copyright (c) 2022 by Adrian Laskowski

Usage: ./sp [opt] "fmt1[fmt2[...]]" val1 [val2 [...]]

  fmt:
    endian indicator (optional):
      @   native endianess (default)
      <   little endian
      >   BIG endian
    type:
      x   pad byte
      c   char
      b   int8_t
      B   uint8_t
      h   int16_t
      H   uint16_t
      i   int32_t
      I   uint32_t
      q   int64_t
      Q   uint64_t
      f   float
      d   double
      s   c string
      p   pascal string
    array (optional):
      use "[N]" array notation to indicate an array of values.
      N is limited up to 65535

  opt:
   -r      reverse - unpack insteadof pack
   -d      debug only. parse enveything but not print output, just debug info.
   -i STR  input stream file (stdin by default). only with -r
   -o STR  output stream file (stdout by default)
   -x XX   pad byte value. ignored for -r.
   -n STR  comma separated struct names for each fmt (exclude x). only with -r
           otherwise skipped, Ex: -n "id,first name,age"
   -p STR  print format for each fmt. only with -r
           fmt: c
             c   char
           fmt: b h i q
             i   decimal signed
             x   hexadecimal (default)
             o   octal
             b   binary
           fmt: B H I Q
             u   decimal unsigned
             x   hexadecimal (default)
             o   octal
             b   binary
           fmt: f d
             f   floating point
             d   double precision floating point
           fmt: s p
             s   string

  val:
    Allow to pass values as numbers, string or bytes.
    Example: 123, 0b111, 0o672, 0xab1, "def ine"
    For -r option, all val's are ignored.


Examples:
$ ./sp -x 0xff "<Hx[2]>Ib[4]s" 0x4142 0xaabbccdd 1 2 3 4 "DEF" | hexdump -C
00000000  42 41 ff ff aa bb cc dd  01 02 03 04 44 45 46 00  |BA..........DEF.|
00000010

$ echo -ne "\x41\x42\x43\x00\xaa\xbb\xcc\xdd\x01\x00\x00\x00" |
 ./sp -r -n "tag,arr,val" "c[3]x>H[2]<I"
tag: ABC
arr: aabb, ccdd
val: 1


```


some example:
```
$ ./sp -r -i sp -n mag,class,data,version,osabi,abiver,e_type,e_machine,e_version,e_entry,e_phoff,e_shoff,e_flags,e_ehsize,e_phentsize,e_phnum,e_shentsize,e_shnum,e_shstrndx "c[4]BBBBBx[7]HHIQQQIHHHHHH"
mag        : ELF
class      : 2
data       : 1
version    : 1
osabi      : 0
abiver     : 0
e_type     : 3
e_machine  : 3e
e_version  : 1
e_entry    : 2ef0
e_phoff    : 40
e_shoff    : 6e20
e_flags    : 0
e_ehsize   : 40
e_phentsize: 38
e_phnum    : d
e_shentsize: 40
e_shnum    : 1f
e_shstrndx : 1e
```

check padd bytes using debug mode:
```
$ ./sp -d -r -i sp -n mag,class,data,version,osabi,abiver,e_type,e_machine,e_version,e_entry,e_phoff,e_shoff,e_flags,e_ehsize,e_phentsize,e_phnum,e_shentsize,e_shnum,e_shstrndx "c[4]BBBBBx[7]HHIQQQIHHHHHH"
endian: @ format:c print_format:' %c' count:4 name:'mag' data size:4 data ptr:0x559d82cc6980
00000000 7f 45 4c 46                                     .ELF            
endian: @ format:B print_format:' %x' count:1 name:'class' data size:1 data ptr:0x559d82cc79b0
00000000 02                                              .               
endian: @ format:B print_format:' %x' count:1 name:'data' data size:1 data ptr:0x559d82cc79d0
00000000 01                                              .               
endian: @ format:B print_format:' %x' count:1 name:'version' data size:1 data ptr:0x559d82cc79f0
00000000 01                                              .               
endian: @ format:B print_format:' %x' count:1 name:'osabi' data size:1 data ptr:0x559d82cc7a10
00000000 00                                              .               
endian: @ format:B print_format:' %x' count:1 name:'abiver' data size:1 data ptr:0x559d82cc7a30
00000000 00                                              .               
endian: @ format:x print_format:'   ' count:7 name:'(null)' data size:7 data ptr:0x559d82cc7a50
00000000 00 00 00 00 00 00 00                            .......         
endian: @ format:H print_format:' %x' count:1 name:'e_type' data size:2 data ptr:0x559d82cc7a70
00000000 03 00                                           ..              
endian: @ format:H print_format:' %x' count:1 name:'e_machine' data size:2 data ptr:0x559d82cc7a90
00000000 3e 00                                           >.              
endian: @ format:I print_format:' %x' count:1 name:'e_version' data size:4 data ptr:0x559d82cc7ab0
00000000 01 00 00 00                                     ....            
endian: @ format:Q print_format:' %x' count:1 name:'e_entry' data size:8 data ptr:0x559d82cc7ad0
00000000 20 2f 00 00 00 00 00 00                          /......        
endian: @ format:Q print_format:' %x' count:1 name:'e_phoff' data size:8 data ptr:0x559d82cc7af0
00000000 40 00 00 00 00 00 00 00                         @.......        
endian: @ format:Q print_format:' %x' count:1 name:'e_shoff' data size:8 data ptr:0x559d82cc7b10
00000000 20 6e 00 00 00 00 00 00                          n......        
endian: @ format:I print_format:' %x' count:1 name:'e_flags' data size:4 data ptr:0x559d82cc7b30
00000000 00 00 00 00                                     ....            
endian: @ format:H print_format:' %x' count:1 name:'e_ehsize' data size:2 data ptr:0x559d82cc7b50
00000000 40 00                                           @.              
endian: @ format:H print_format:' %x' count:1 name:'e_phentsize' data size:2 data ptr:0x559d82cc7b70
00000000 38 00                                           8.              
endian: @ format:H print_format:' %x' count:1 name:'e_phnum' data size:2 data ptr:0x559d82cc7b90
00000000 0d 00                                           ..              
endian: @ format:H print_format:' %x' count:1 name:'e_shentsize' data size:2 data ptr:0x559d82cc7bb0
00000000 40 00                                           @.              
endian: @ format:H print_format:' %x' count:1 name:'e_shnum' data size:2 data ptr:0x559d82cc7bd0
00000000 1f 00                                           ..              
endian: @ format:H print_format:' %x' count:1 name:'e_shstrndx' data size:2 data ptr:0x559d82cc7bf0
00000000 1e 00                                           ..              
```

