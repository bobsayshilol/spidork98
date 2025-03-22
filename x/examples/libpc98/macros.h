#ifndef DEFINES_H
#define DEFINES_H

// Utility macros.
#define QUOTE(x) #x
#define STRINGIFY(x) QUOTE(x)
#define CONCAT2(a, b) a ## b
#define CONCAT(a, b) CONCAT2(a, b)

// Mark a function as passing by registers first.
#define FASTCALL __attribute__((regparm (3)))

// Force inline a function.
#define FORCEINLINE __inline__

// Align type to N bytes.
#define ALIGNAS(n) __attribute__((aligned (n)))

// Make a function or variable as unused.
#define UNUSED __attribute__((unused))

// Check something at compile time.
#define STATIC_ASSERT(x) struct CONCAT(_static_assert, __LINE__) \
    { char static_assert_failed[(x) ? 1 : -1]; }

#endif
