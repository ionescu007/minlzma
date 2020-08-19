#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <minlzma.h>

int32_t
main (
    int32_t ArgumentCount,
    char* Arguments[]
    )
{
    FILE* inputFile;
    FILE* outputFile;
    size_t fileSize;
    size_t sizeRead;
    uint32_t inputSize, outputSize;
    uint8_t* inputBuffer;
    uint8_t* outputBuffer;
    struct stat stat;
    bool decodeResult;

    inputFile = NULL;
    outputFile = NULL;
    inputBuffer = NULL;
    outputBuffer = NULL;

    printf("minlzdec v.1.1.1 -- http://ionescu007.github.io/minlzma\n");
    printf("Copyright(c) 2020 Alex Ionescu (@aionescu)\n\n");
    if (ArgumentCount != 3)
    {
        printf("Usage: minlzdec [INPUT FILE] [OUTPUT FILE]\n");
        printf("Decompress INPUT FILE in the .xz format into OUTPUT FILE.\n");
        errno = EINVAL;
        goto Cleanup;
    }

    inputFile = fopen(Arguments[1], "rb");
    if (inputFile == 0)
    {
        printf("Failed to open input file: %s\n", Arguments[1]);
        goto Cleanup;
    }

    fstat(fileno(inputFile), &stat);
    fileSize = stat.st_size;
    printf("Input file size: %zd\n", fileSize);

    inputBuffer = malloc(fileSize);
    if (inputBuffer == NULL)
    {
        printf("Out of memory for allocating input buffer\n");
        goto Cleanup;
    }

    sizeRead = fread(inputBuffer, 1, fileSize, inputFile);
    if (sizeRead != fileSize)
    {
        printf("File read failed (%zd vs %zd bytes\n", sizeRead, fileSize);
        goto Cleanup;
    }

    inputSize = (uint32_t)fileSize;
    outputSize = 0;
    decodeResult = XzDecode(inputBuffer, inputSize, outputBuffer, &outputSize);
    if (decodeResult == false)
    {
        printf("Decoding failed after %d bytes\n", outputSize);
        errno = ENOTSUP;
        goto Cleanup;
    }

    printf("Decompressed file will be %d bytes (%f%% ratio)\n",
            outputSize, (double)inputSize / (double)outputSize);

    outputBuffer = malloc(outputSize);
    if (outputBuffer == NULL)
    {
        printf("Out of memory for allocating output buffer\n");
        goto Cleanup;
    }

    decodeResult = XzDecode(inputBuffer, inputSize, outputBuffer, &outputSize);
    if (decodeResult == false)
    {
        printf("Decoding failed after %d bytes\n", outputSize);
        errno = ENOTSUP;
        goto Cleanup;
    }

    printf("Decompressed %d bytes\n", outputSize);

    outputFile = fopen(Arguments[2], "wb");
    if (outputFile == 0)
    {
        printf("Failed to open output file: %s\n", Arguments[1]);
        goto Cleanup;
    }

    fileSize = fwrite(outputBuffer, 1, outputSize, outputFile);
    if (fileSize != outputSize)
    {
        printf("File write failed (%zd vs %d bytes)\n", fileSize, outputSize);
        goto Cleanup;
    }
    errno = 0;

Cleanup:
    if (outputBuffer != NULL)
    {
        free(outputBuffer);
    }
    if (inputBuffer != NULL)
    {
        free(inputBuffer);
    }

    if (outputFile != 0)
    {
        fflush(outputFile);
        fclose(outputFile);
    }

    if (inputFile != 0)
    {
        fclose(inputFile);
    }

    return errno;
}
