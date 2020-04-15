# Introduction
The Minimal LZMA (`minlzma`) project aims to provide a minimalistic, cross-platform, highly commented, standards-compliant C library (`minlzlib`) for decompressing LZMA2-encapsulated compressed data in LZMA format within an XZ container, as can be generated with Python 3.6, 7-zip, and xzutils. Additionally, a simple, portable, command-line tool (`minlzdec`) is provided for excercising the functionality on a provided input file.
{: .text-justify}

# External Interface

```
bool
XzDecode (
    uint8_t* InputBuffer,
    uint32_t InputSize,
    uint8_t* OutputBuffer,
    uint32_t* OutputSize
    );
```

# Compile-time Options

`MINLZ_INTEGRITY_CHECKS` -- This
`MINLZ_META_CHECKS` -- This