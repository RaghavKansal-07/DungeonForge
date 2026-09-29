#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool Serialization_Write(
    const char *filename,
    const void *data,
    size_t size
);

bool Serialization_Read(
    const char *filename,
    void *data,
    size_t size
);

#endif