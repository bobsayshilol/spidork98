#ifndef MEMORY_H
#define MEMORY_H

#include <cstdlib>

namespace memory {

// Allocate with an alignment of 4 bytes.
void *alloc4(size_t size);

// Free a 4 byte aligned allocation.
void free4(void *ptr);

} // namespace memory

#endif
