# sp
Binary struct pack and unpack. Based on python's struct.pack and struct.unpack

```
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

  opt:
   -r      reverse - unpack insteadof pack
   -i STR  input stream file (stdin by default). only with -r
   -o STR  output stream file (stdout by default)
   -x XX   pad byte value. ignored for -r.
   -n STR  comma separated struct names for each fmt (exclude x). only with -r
           otherwise skipped, Ex: -n "id,first name,age"
   -p STR  print format for each fmt. only with -r
           fmt: c
             c   char
           fmt: b B h H i I l L q Q P
             i   decimal signed
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
    In case where -r is passed as option, then all val's are ignored


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

