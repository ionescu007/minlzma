/*++

Copyright (c) Alex Ionescu.  All rights reserved.

Module Name:

    xzstream.c

Abstract:

    This module implements the XZ stream format decoding, including support for
    parsing the stream header and block header, and then handing off the block
    decoding to the LZMA2 decoder. Finally, if "meta checking" is enabled, then
    the index and stream footer are also parsed and validated. Optionally, each
    of these component structures can be checked against its CRC32 checksum, if
    "integrity checking" has been enabled. Note that this library only supports
    single-stream, single-block XZ files that have CRC32 (or None) set as their
    block checking algorithm. Finally, no BJC filters are supported, and files
    with a compressed/uncompressed size metadata indicator are not handled.

Author:

    Alex Ionescu (@aionescu) 15-Apr-2020 - Initial version

Environment:

    Windows & Linux, user mode and kernel mode.

--*/

#include "minlzlib.h"
#include "xzstream.h"

#ifdef MINLZ_META_CHECKS
//
// XZ Stream Container State
//
typedef struct _CONTAINER_STATE
{
    //
    // Size of the XZ header and the index, used to validate against footer
    //
    uint32_t HeaderSize;
    uint32_t IndexSize;
    //
    // Size of the compressed block and its checksum
    //
    uint32_t UncompressedBlockSize;
    uint32_t UnpaddedBlockSize;
    uint32_t ChecksumSize;
} CONTAINER_STATE, * PCONTAINER_STATE;
CONTAINER_STATE g_Container;
PCONTAINER_STATE Container = &g_Container;
#endif

#ifdef MINLZ_META_CHECKS
bool
XzDecodeVli (
    vli_type* Vli
    )
{
    uint8_t vliByte;
    uint32_t bitPos;

    //
    // Read the initial VLI byte (might be the value itself)
    //
    if (!BfRead(&vliByte))
    {
        return false;
    }
    *Vli = vliByte & 0x7F;

    //
    // Check if this was a complex VLI (and we have space for it)
    //
    bitPos = 7;
    while ((vliByte & 0x80) != 0)
    {
        //
        // Read the next byte
        //
        if (!BfRead(&vliByte))
        {
            return false;
        }

        //
        // Make sure we're not decoding an invalid VLI
        //
        if ((bitPos == (7 * VLI_BYTES_MAX)) || (vliByte == 0))
        {
            return false;
        }

        //
        // Decode it and move to the next 7 bits
        //
        *Vli |= (vli_type)((vliByte & 0x7F) << bitPos);
        bitPos += 7;
    }
    return true;
}

bool
XzDecodeIndex (
    void
    )
{
    uint32_t vli;
    uint8_t* indexStart;
    uint8_t* indexEnd;
    uint32_t* pCrc32;
    uint8_t indexByte;

    //
    // Remember where the index started so we can compute its size
    //
    BfSeek(0, &indexStart);

    //
    // The index always starts out with an empty byte
    //
    if (!BfRead(&indexByte) || (indexByte != 0))
    {
        return false;
    }

    //
    // Then the count of blocks, which we expect to be 1
    //
    if (!XzDecodeVli(&vli) || (vli != 1))
    {
        return false;
    }

    //
    // Then the unpadded block size, which should match
    //
    if (!XzDecodeVli(&vli) || (Container->UnpaddedBlockSize != vli))
    {
        return false;
    }

    //
    // Then the uncompressed block size, which should match
    //
    if (!XzDecodeVli(&vli) || (Container->UncompressedBlockSize != vli))
    {
        return false;
    }

    //
    // Then we pad to the next multiple of 4
    //
    if (!BfAlign())
    {
        return false;
    }

    //
    // Store the index size with padding to validate the footer later
    //
    BfSeek(0, &indexEnd);
    Container->IndexSize = (uint32_t)(indexEnd - indexStart);

    //
    // Read the CRC32, which is not part of the index size
    //
    if (!BfSeek(sizeof(*pCrc32), (uint8_t**)&pCrc32))
    {
        return false;
    }
#ifdef MINLZ_INTEGRITY_CHECKS
    //
    // Make sure the index is not corrupt
    //
    if (Crc32(indexStart, Container->IndexSize) != *pCrc32)
    {
        return false;
    }
#endif
    return true;
}

bool
XzDecodeStreamFooter (
    void
    )
{
    PXZ_STREAM_FOOTER streamFooter;

    //
    // Seek past the footer, making sure we have space in the input stream
    //
    if (!BfSeek(sizeof(*streamFooter), (uint8_t**)&streamFooter))
    {
        return false;
    }

    //
    // Validate the footer magic
    //
    if (streamFooter->Magic != 'ZY')
    {
        return false;
    }

    //
    // Validate no flags other than checksum type are set
    //
    if ((streamFooter->Flags != 0) &&
        ((streamFooter->CheckType != XzCheckTypeCrc32) &&
         (streamFooter->CheckType != XzCheckTypeNone)))
    {
        return false;
    }

    //
    // Validate if the footer accurately describes the size of the index
    //
    if (Container->IndexSize != (streamFooter->BackwardSize * 4))
    {
        return false;
    }
#ifdef MINLZ_INTEGRITY_CHECKS
    //
    // Compute the footer's CRC32 and make sure it's not corrupted
    //
    if (Crc32(&streamFooter->BackwardSize,
               sizeof(streamFooter->BackwardSize) +
               sizeof(streamFooter->Flags)) !=
        streamFooter->Crc32)
    {
        return false;
    }
#endif
    return true;
}
#endif

bool
XzDecodeBlock (
    uint8_t* OutputBuffer,
    uint32_t* BlockSize
    )
{
#ifdef MINLZ_META_CHECKS
    uint8_t *inputStart, *inputEnd;
#endif
    //
    // Decode the LZMA2 stream. If full integrity checking is enabled, also
    // save the offset before and after decoding, so we can save the block
    // sizes and compare them against the footer and index after decoding.
    //
#ifdef MINLZ_META_CHECKS
    BfSeek(0, &inputStart);
#endif
    if (!Lz2DecodeStream(BlockSize))
    {
        return false;
    }
#ifdef MINLZ_META_CHECKS
    BfSeek(0, &inputEnd);
    Container->UnpaddedBlockSize = Container->HeaderSize +
                                   (uint32_t)(inputEnd - inputStart);
    Container->UncompressedBlockSize = *BlockSize;
#endif
    //
    // After the block data, we need to pad to 32-bit alignment
    //
    if (!BfAlign())
    {
        return false;
    }
#if defined(MINLZ_INTEGRITY_CHECKS) || defined(MINLZ_META_CHECKS)
    //
    // Finally, move past the size of the checksum if any, then compare it with
    // with the actual CRC32 of the block, if integrity checks are enabled. If
    // meta checks are enabled, update the block size so the index checking can
    // validate it.
    //
    if (!BfSeek(Container->ChecksumSize, &inputEnd))
    {
        return false;
    }
#endif
#ifdef MINLZ_INTEGRITY_CHECKS
    if (Crc32(OutputBuffer, *BlockSize) != *(uint32_t*)inputEnd)
    {
        return false;
    }
#endif
#ifdef MINLZ_META_CHECKS
    Container->UnpaddedBlockSize += Container->ChecksumSize;
#endif
    return true;
}

bool
XzDecodeStreamHeader (
    void
    )
{
    PXZ_STREAM_HEADER streamHeader;

    //
    // Seek past the header, making sure we have space in the input stream
    //
    if (!BfSeek(sizeof(*streamHeader), (uint8_t**)&streamHeader))
    {
        return false;
    }
#ifdef MINLZ_META_CHECKS
    //
    // Validate the header magic
    //
    if ((*(uint32_t*)&streamHeader->Magic[1] != 'ZXz7') ||
        (streamHeader->Magic[0] != 0xFD) ||
        (streamHeader->Magic[5] != 0x00))
    {
        return false;
    }

    //
    // Validate no flags other than checksum type are set
    //
    if ((streamHeader->Flags != 0) &&
        ((streamHeader->CheckType != XzCheckTypeCrc32) &&
         (streamHeader->CheckType != XzCheckTypeNone)))
    {
        return false;
    }

    //
    // Remember that a checksum might come at the end of the block later
    //
    Container->ChecksumSize = streamHeader->CheckType * 4;
#endif
#ifdef MINLZ_INTEGRITY_CHECKS
    //
    // Compute the header's CRC32 and make sure it's not corrupted
    //
    if (Crc32(&streamHeader->Flags, sizeof(streamHeader->Flags)) !=
        streamHeader->Crc32)
    {
        return false;
    }
#endif
    return true;
}

bool
XzDecodeBlockHeader (
    uint32_t OutputSize
    )
{
    PXZ_BLOCK_HEADER blockHeader;
#ifdef MINLZ_META_CHECKS
    uint32_t size;
#endif
    //
    // Seek past the header, making sure we have space in the input stream
    //
    if (!BfSeek(sizeof(*blockHeader), (uint8_t**)&blockHeader))
    {
        return false;
    }
#ifdef MINLZ_META_CHECKS
    //
    // Validate that the size of the header is what we expect
    //
    Container->HeaderSize = (blockHeader->Size + 1) * 4;
    if (Container->HeaderSize != sizeof(*blockHeader))
    {
        return false;
    }

    //
    // Validate that no additional flags or filters are enabled
    //
    if (blockHeader->Flags != 0)
    {
        return false;
    }

    //
    // Validate that the only filter is the LZMA2 filter
    //
    if (blockHeader->LzmaFlags.Id != 0x21)
    {
        return false;
    }

    //
    // With the expected number of property bytes
    //
    if (blockHeader->LzmaFlags.Size != sizeof(blockHeader->LzmaFlags.Properties))
    {
        return false;
    }

    //
    // The only property is the dictionary size, make sure it is valid
    //
    size = blockHeader->LzmaFlags.DictionarySize;
    if (size > 39)
    {
        return false;
    }

    //
    // And make sure it isn't larger than the output buffer
    //
    size = (2 + (size & 1)) << ((size >> 1) + 11);
    if (size > OutputSize)
    {
        return false;
    }
#else
    (void)(OutputSize);
#endif
#ifdef MINLZ_INTEGRITY_CHECKS
    //
    // Compute the header's CRC32 and make sure it's not corrupted
    //
    if (Crc32(blockHeader,
              Container->HeaderSize - sizeof(blockHeader->Crc32)) !=
        blockHeader->Crc32)
    {
        return false;
    }
#endif
    return true;
}

bool
XzDecode (
    uint8_t* InputBuffer,
    uint32_t InputSize,
    uint8_t* OutputBuffer,
    uint32_t* OutputSize
    )
{
    //
    // Initialize the input buffer descriptor and history buffer (dictionary)
    //
    BfInitialize(InputBuffer, InputSize);
    DtInitialize(OutputBuffer, *OutputSize);

    //
    // Decode the stream header for validity
    //
    if (!XzDecodeStreamHeader())
    {
        return false;
    }

    //
    // Decode the block header to know the dictionary size
    //
    if (!XzDecodeBlockHeader(*OutputSize))
    {
        return false;
    }

    //
    // Decode the actual block
    //
    if (!XzDecodeBlock(OutputBuffer, OutputSize))
    {
        return false;
    }
#ifdef MINLZ_META_CHECKS
    //
    // Decode the index for validity checks
    //
    if (!XzDecodeIndex())
    {
        return false;
    }

    //
    // And finally decode the footer as a final set of checks
    //
    if (!XzDecodeStreamFooter())
    {
        return false;
    }
#endif
    return true;
}
