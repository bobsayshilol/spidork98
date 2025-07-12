#ifndef MEMORY_H
#define MEMORY_H

#include "macros.h"

#include <cstdlib>

namespace memory {

// Allocate with an alignment of 4 bytes.
void *alloc4(size_t size);

template <typename T>
T *alloc4(size_t size) {
    STATIC_ASSERT(sizeof(T) == 1);
    return static_cast<T*>(alloc4(size));
}

// Free a 4 byte aligned allocation.
void free4(void *ptr);

} // namespace memory

#endif
