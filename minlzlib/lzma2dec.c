/*++

Copyright (c) Alex Ionescu.  All rights reserved.

Module Name:

    lzma2dec.c

Abstract:

    This module implements the LZMA2 decoding logic responsible for parsing the
    LZMA2 Control Byte, the Information Bytes (Compressed & Uncompressed Stream
    Size), and the Property Byte during the initial Dictionary Reset. Note that
    this module only implements support for a single such reset (i.e.: archives
    in "solid" mode).

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
    uint32_t RawSize;
    uint16_t CompressedSize;
} CHUNK_STATE, * PCHUNK_STATE;
CHUNK_STATE Chunk;

bool
Lz2DecodeChunk (
    uint32_t* BytesProcessed
    )
{
    uint32_t bytesProcessed;
    //
    // Make sure we always have space for the biggest possible LZMA sequence
    //
    if (Chunk.CompressedSize < LZMA_MAX_SEQUENCE_SIZE)
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
        (bytesProcessed != Chunk.CompressedSize))
    {
        return false;
    }

    //
    // The entire output stream must have been written to, and the dictionary
    // must be full now.
    //
    if (!DtIsComplete(&bytesProcessed) || (bytesProcessed != Chunk.RawSize))
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
    uint8_t* inBytes;
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
        // Read the appropriate number of info bytes based on the stream type.
        //
        if (!BfSeek((controlByte.u.Common.IsLzma ==1 )? 4 : 2, &inBytes))
        {
            break;
        }

        //
        // For LZMA streams calculate both the uncompressed and compressed size
        // from the info bytes. Uncompressed streams only have the former.
        //
        if (controlByte.u.Common.IsLzma == 1)
        {
            Chunk.RawSize = controlByte.u.Lzma.RawSize << 16;
            Chunk.CompressedSize = inBytes[2] << 8;
            Chunk.CompressedSize += inBytes[3] + 1;
        }
        else
        {
            Chunk.RawSize = 0;
        }

        //
        // Make sure that the output buffer that was supplied is big enough to
        // fit the uncompressed chunk, unless we're just calculating the size.
        //
        Chunk.RawSize += inBytes[0] << 8;
        Chunk.RawSize += inBytes[1] + 1;
        if (!GetSizeOnly && !DtSetLimit(Chunk.RawSize))
        {
            break;
        }

        //
        // Check if the full LZMA state needs to be reset, which must happen at
        // the start of stream. Also check for a property reset, which occurs
        // when an LZMA stream follows an uncompressed stream.
        //
        if ((controlByte.u.Lzma.ResetState == Lzma2FullReset) ||
            (controlByte.u.Lzma.ResetState == Lzma2PropertyReset))
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
            *BytesProcessed += Chunk.RawSize;
            BfSeek((controlByte.u.Common.IsLzma == 1) ? Chunk.CompressedSize :
                                                        Chunk.RawSize,
                   &inBytes);
            continue;
        }
        else if (controlByte.u.Common.IsLzma == 0)
        {
            //
            // Seek to the requested size in the input buffer
            //
            if (!BfSeek(Chunk.RawSize, &inBytes))
            {
                return false;
            }

            //
            // Copy the data into the dictionary as-is
            //
            for (uint32_t i = 0; i < Chunk.RawSize; i++)
            {
                DtPutSymbol(inBytes[i]);
            }

            //
            // Update bytes and keep going to the next chunk
            //
            *BytesProcessed += Chunk.RawSize;
            continue;
        }

        //
        // Read the initial range and code bytes to initialize the arithmetic
        // coding decoder, and let it know how much input data exists. We've
        // already validated that this much space exists in the input buffer.
        //
        if (!RcInitialize(&Chunk.CompressedSize))
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
