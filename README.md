# Minimal LZMA Project (`minlzma`)
[<img align="left" src="minlzma.png" width="72"/>](minlzma.png)
The Minimal LZMA (`minlzma`) project aims to provide a minimalistic, cross-platform, highly commented, standards-compliant C library (`minlzlib`) for decompressing LZMA2-encapsulated compressed data in LZMA format within an XZ container, as can be generated with Python 3.6, 7-zip, and xzutils. Additionally, a simple, portable, command-line tool (`minlzdec`) is provided for excercising the functionality on a provided input file.

# External Interface

~~~ c
/*!
 * @brief          Decompresses an XZ stream from InputBuffer into OutputBuffer.
 *
 * @detail         The XZ stream must contain a single block with an LZMA2 filter
 *                 and no BJC2 filters, using default LZMA properties, and using
 *                 either CRC32 or None as the checksum type.
 *
 * @param[in]      InputBuffer - A fully formed buffer containing the XZ stream.
 * @param[in]      InputSize - The size of the input buffer.
 * @param[in]      OutputBuffer - A fully allocated buffer to receive the output.
 *                 Callers can pass in NULL if they do not intend to decompress,
 *                 in combination with setting OutputSize to 0, in order to query
 *                 the final expected size of the decompressed buffer.
 * @param[in,out]  OutputSize - On input, the size of the buffer. On output, the
 *                 size of the decompressed result.
 *
 * @return         true - The input buffer was fully decompressed in OutputBuffer,
 *                 or no decompression was requested, the size of the decompressed
 *                 buffer was returned in OutputSIze.
 *                 false - A failure occurred during the decompression process.
 */
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

* The entire input stream must be available (multi-call/streaming mode are not supported)
* The entire output buffer must be allocated with a fixed size -- however, callers are able to query the required size
* The XZ file must be "solid", i.e.: a single block (with a single dictionary/properties reset)
* The LZMA2 property byte must indicate the LZMA properties `lc = 3`, `pb = 2`, `lc = 0`
* The XZ block must not have the optional "compressed size" and/or "uncompressed size" VLI metadata

Note that while these assumptions may seem overly restrictive, they correspond to the usual files produced by `xzutils`, `7-zip` when choosing XZ as the format, and the `Python` `LZMA` module. Most encoders do not support the vast majority of XZ/LZMA2's purported capabilities, such as SHA256 or CRC64, multiple blocks, etc.

# Testing (Linux)

* Generate 4MB of noise :
  - `shasum` input file
  - Compress with `Python`
  - Compress with `xzutils`
  - Decompress with `minlzdec`
  - `shasum` output files
  
* Generate 4MB of whitespace:
  - `shasum` input file
  - Compress with `Python`
  - Compress with `xzutils`
  - Decompress with `minlzdec`
  - `shasum` output files

# Testing (Windows)

* Generate 4MB of noise:
  - `Get-FileHash` input file
  - Compress with `7z`
  - Decompress with `minlzdec`
  - `Get-FileHash` output file
  
* Generate 4MB of whitespace:
  - `Get-FileHash` input file
  - Compress with `7z`
  - Decompress with `minlzdec`
  - `Get-FileHash` output file

# Compile-time Options
* `MINLZ_INTEGRITY_CHECKS` -- This option configures whether or not CRC32 checksumming of the XZ data structures and compressed block should be performed, or skipped. Removing this functionality gains an increase in performance which scales with the size of the input file. It results in a minimal increase in library size, and also requires the implementation of a platform-specific CRC32 checksum function that correponds to the following prototype: `uint32_t OsComputeCrc32(uint32_t Initial, const uint8_t* Data, uint32_t Length);`

* `MINLZ_META_CHECKS` -- This option configures whether or nor the input files should be fully trusted to conform to the requirements of `minlzlib` and do not require checking the various stream header flags or block header flags and other attributes. Additionally, the index and stream footer are completely ignored. This mode results in a sub-10KB library that can decode 100MB/s on a ~3.6GHz single-processor. This is only recommended if the input file is wrapped or delivered in a cryptographically tamper-proof secure channel or container (such as a signed hash).

# Usage
TODO

# Build Instructions
Within Visual Studio 2019, you can use File->Open->CMake and point it at the top-level `CMakeFiles.txt`, and choose either the `win-amd64` target or the `win-release-amd64` target. The former builds a binary with no optimizations, the later builds a fully optimized binary (for speed) with debug symbols.

If you use WSL, ...

For Linux native builds, ...

# Acknowledgments
The author would like to thank the shoulders of the following giants, whose code, documentation, and writing was monumental in this effort:

* Antonio Diaz Diaz -- author of lzip
* Charles Bloom -- author of oodle
* Fabian Giesen -- author of oodle
* Igor Pavlov -- author of 7-zip
* Lasse Collin -- author of xzutils

The author would also like to thank the following reviwers for identifying various bugs, typos, and other improvements:

* Satoshi Tanda -- https://github.com/tandasat
* Daniel Martin --
* Thomas Fabber -- https://github.com/ThFabba
