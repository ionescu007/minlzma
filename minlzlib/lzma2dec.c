/*++

Copyright (c) Alex Ionescu.  All rights reserved.

Module Name:

    lzma2dec.c

Abstract:

    This module implements the LZMA2 decoding logic responsible for parsing the
    LZMA2 Control Byte, the Information Bytes (Compressed & Uncompressed Stream
    Size), and the Property Byte during the initial Dictionary Reset. Note that
    this module only implements support for a single such reset. Finally, only
    streams that are fully LZMA compressed are supported -- uncompressed blocks
    are not handled.

Author:

    Alex Ionescu (@aionescu) 15-Apr-2020 - Initial version

Environment:

    Windows & Linux, user mode and kernel mode.

--*/

#include "minlzlib.h"
#include "lzma2dec.h"

//
// LZMA2 Chunk State
//
typedef struct _CHUNK_STATE
{
    //
    // Compressed and uncompressed size of the LZMA chunk being decoded
    //
    uint32_t UncompressedSize;
    uint16_t CompressedSize;
} CHUNK_STATE, * PCHUNK_STATE;
CHUNK_STATE ChunkState;

bool
Lz2DecodeChunk (
    uint32_t* BytesProcessed
    )
{
    uint32_t bytesProcessed;
    //
    // Make sure we always have space for the biggest possible LZMA sequence
    //
    if (ChunkState.CompressedSize < LZMA_MAX_SEQUENCE_SIZE)
    {
        return false;
    }

    //
    // Go and decode this chunk, sequence by sequence
    //
    if (!LzDecode())
    {
        return false;
    }

    //
    // In a correctly formatted stream, the last arithmetic-coded sequence must
    // be zero once we finished with the last chunk. Make sure the stream ended
    // exactly where we expected it to.
    //
    if (!RcIsComplete(&bytesProcessed) ||
        (bytesProcessed != ChunkState.CompressedSize))
    {
        return false;
    }

    //
    // The entire output stream must have been written to, and the dictionary
    // must be full now.
    //
    if (!DtIsComplete(&bytesProcessed) ||
        (bytesProcessed != ChunkState.UncompressedSize))
    {
        return false;
    }
    *BytesProcessed += bytesProcessed;
    return true;
}

bool
Lz2DecodeStream (
    uint32_t* BytesProcessed,
    bool GetSizeOnly
    )
{
    uint8_t (*pInfoBytes)[4];
    LZMA2_CONTROL_BYTE controlByte;
    uint8_t propertyByte;

    //
    // Read the first control byte
    //
    *BytesProcessed = 0;
    while (BfRead(&controlByte.Value))
    {
        //
        // When the LZMA2 control byte is 0, the entire stream is decoded. This
        // is the only success path out of this function.
        //
        if (controlByte.Value == 0)
        {
            return true;
        }

        //
        // We only support compressed streams, not uncompressed ones
        //
        if (controlByte.u.Common.IsLzma == 0)
        {
            break;
        }

        //
        // Read the information bytes
        //
        if (!BfSeek(sizeof(*pInfoBytes), (uint8_t**)&pInfoBytes))
        {
            break;
        }

        //
        // Decode the 1-based big-endian uncompressed size. It must fit into
        // the output buffer that was supplied, unless we're just getting size.
        //
        ChunkState.UncompressedSize = controlByte.u.Lzma.UncompressedSize << 16;
        ChunkState.UncompressedSize += (*pInfoBytes)[0] << 8;
        ChunkState.UncompressedSize += (*pInfoBytes)[1] + 1;
        if (!GetSizeOnly && !DtSetLimit(ChunkState.UncompressedSize))
        {
            break;
        }

        //
        // Decode the 1-based big-endian compressed size. It must fit into the
        // input buffer that was given, which RcInitialize will verify below.
        //
        ChunkState.CompressedSize = (*pInfoBytes)[2] << 8;
        ChunkState.CompressedSize += (*pInfoBytes)[3] + 1;

        //
        // Check if the full LZMA state needs to be reset, which must happen at
        // the start of stream. Other reset states are not supported.
        //
        if (controlByte.u.Lzma.ResetState == Lzma2FullReset)
        {
            //
            // Read the LZMA properties and then initialize the decoder.
            //
            if (!BfRead(&propertyByte) || !LzInitialize(propertyByte))
            {
                break;
            }
        }
        else if (controlByte.u.Lzma.ResetState != Lzma2NoReset)
        {
            break;
        }

        //
        // Don't do any decompression if the caller only wants to know the size
        //
        if (GetSizeOnly)
        {
            *BytesProcessed += ChunkState.UncompressedSize;
            BfSeek(ChunkState.CompressedSize, (uint8_t**)&pInfoBytes);
            continue;
        }

        //
        // Read the initial range and code bytes to initialize the arithmetic
        // coding decoder, and let it know how much input data exists. We've
        // already validated that this much space exists in the input buffer.
        //
        if (!RcInitialize(&ChunkState.CompressedSize))
        {
            break;
        }

        //
        // Start decoding the LZMA sequences in this chunk
        //
        if (!Lz2DecodeChunk(BytesProcessed))
        {
            break;
        }
    }
    return false;
}
