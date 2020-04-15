# Minimal LZMA Project (`minlzma`)

The Minimal LZMA (`minlzma`) project aims to provide a minimalistic, cross-platform, highly commented, standards-compliant C library (`minlzlib`) for decompressing LZMA2-encapsulated compressed data in LZMA format within an XZ container, as can be generated with Python 3.6, 7-zip, and xzutils. Additionally, a simple, portable, command-line tool (`minlzdec`) is provided for excercising the functionality on a provided input file.

# External Interface

~~~ c
bool
XzDecode (
    uint8_t* InputBuffer,
    uint32_t InputSize,
    uint8_t* OutputBuffer,
    uint32_t* OutputSize
    );
~~~

# Limitations and Restrictions
In order to provide its vast simplicity, fast performance, minimal source, and small compiled size, `minlzlib` makes certain assumptions about the input file and has certain restrictions or limitations:

* The LZMA properties must be `lc = 3`, `pb = 2`, `lc = 0`
* The XZ file must have a single stream, with a single block (with a single dictionary/properties reset)
* The LZMA2 chunks must all be compressed (LZMA2 uncompressed chunks are not supported)
* TODO

# Compile-time Options
* `MINLZ_INTEGRITY_CHECKS` -- This option configures whether or not CRC32 checksumming of the XZ data structures and compressed block should be performed, or skipped. Removing this functionality gains an increase in performance which scales with the size of the input file. It results in a minimal increase in library size, and also requires the implementation of a platform-specific CRC32 checksum function that correponds to the following prototype: `uint32_t OsComputeCrc32(uint32_t Initial, const uint8_t* Data, uint32_t Length);`

* `MINLZ_META_CHECKS` -- This option configures whether or nor the input files should be fully trusted to conform to the requirements of `minlzlib` and do not require checking the various stream header flags or block header flags and other attributes. Additionally, the index and stream footer are completely ignored. This mode results in a sub-10KB library that can decode 100MB/s on a ~3.6GHz single-processor. This is only recommended if the input file is wrapped or delivered in a cryptographically tamper-proof secure channel or container (such as a signed hash).

# Usage
TODO

# Build Instructions
TODO
