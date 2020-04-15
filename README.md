# Introduction
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
* 

# Compile-time Options
* `MINLZ_INTEGRITY_CHECKS` -- This option configures whether or not CRC32 checksumming of the XZ data structures and compressed block should be performed, or skipped. Removing this functionality gains an increase in performance which scales with the size of the input file. It results in a minimal increase in library size, and also requires the implementation of a platform-specific CRC32 checksum function that correponds to the following prototype: `uint32_t OsComputeCrc32(uint32_t Initial, const uint8_t* Data, uint32_t Length);`

* `MINLZ_META_CHECKS` -- This option configures whether or nor the input files should be fully trusted to conform to the requirements of `minlzlib` and do not require checking the various stream header flags or block header flags and other attributes. Additionally, the index and stream footer are completely ignored. This mode results in a sub-10KB library that can decode 100MB/s on a ~3.6GHz single-processor. This is only recommended if the input file is wrapped or delivered in a cryptographically tamper-proof secure channel or container (such as a signed hash).

Paragraph
{: .class .class-1 .class-2}

Paragraph
{: #custom-id}

Paragraph
{: .class .class-1 #custom-id-1}

## Heading
{: .class .class-1 #custom-id-2}

Paragraph
{: .class #custom-id-3 style="padding-top:0" key="value"}

## Heading {#hello}

List:

- {: .class} List item with custom class
- {:#id} List item with custom id

To a [link]{: #link}, in-line.

This is a [link][google-es]{:hreflang="es"} in Spanish.

[link]: https://google.com
[google-es]: https://google.es

Center-aligned
{: .alert .alert-info .text-center}
Paragraph markup:
Center-aligned
{: .text-center}
Heading markup:
### Center-aligned
{: .text-center}
Align to the right
Right-aligned
Right-aligned
{: .alert .alert-info .text-right}
Justify
Justified
Justified
{: .alert .alert-info .text-justify}

Mix HTML + Markdown Markup
Mixing HTML markup with markdown is something almost "unthinkable" to someone used to regular markdown. And it's all over this document!
Use the following markup at the beginning of your document:
{::options parse_block_html="true" /}
And feel free to mix everything up:
Something in **markdown**.

<p>Then an HTML tag with crazy **markup** _all over_ the place!</p>
Output
Something in markdown.
Then an HTML tag with crazy markup all over the place!
You can close the markup parser tag at any point, if you want to:
{::options parse_block_html="false" /}

