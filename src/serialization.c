#include "serialization.h"

#include <stdio.h>

bool Serialization_Write(
    const char *filename,
    const void *data,
    size_t size
)
{
    if (filename == NULL ||
        data == NULL ||
        size == 0)
    {
        return false;
    }

    FILE *file =
        fopen(
            filename,
            "wb"
        );

    if (file == NULL)
        return false;

    size_t bytes_written =
        fwrite(
            data,
            1,
            size,
            file
        );

    if (fclose(file) != 0)
        return false;

    return bytes_written == size;
}


bool Serialization_Read(
    const char *filename,
    void *data,
    size_t size
)
{
    if (filename == NULL ||
        data == NULL ||
        size == 0)
    {
        return false;
    }

    FILE *file =
        fopen(
            filename,
            "rb"
        );

    if (file == NULL)
        return false;

    size_t bytes_read =
        fread(
            data,
            1,
            size,
            file
        );

    if (fclose(file) != 0)
        return false;

    return bytes_read == size;
}