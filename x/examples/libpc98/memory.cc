#include "memory.h"
#include "maths.h"

namespace memory {

#ifdef WEB_BUILD
typedef u64 Addr;
#else
typedef u32 Addr;
#endif

// Really lazy, doubt we'll hit allocation limits.
//
// | pad | size8 | data |
// ^     ^       ^
// |     |       returned address
// |     padding size + 1 (u8)
// raw allocation address
//

void *alloc4(size_t size) {
    u8 *addr = reinterpret_cast<u8*>(malloc(maths::pad_to<4>(size) + 4));
    if (!addr) {
        return addr;
    }

    const u8 alignment = reinterpret_cast<Addr>(addr) & 3;
    const u8 size8 = 4 - alignment;
    addr += size8;
    addr[-1] = size8;
    return addr;
}

void free4(void *ptr) {
    if (ptr) {
        u8 *addr = reinterpret_cast<u8*>(ptr);
        const u8 size8 = addr[-1];
        addr -= size8;
        free(addr);
    }
}

} // namespace memory

