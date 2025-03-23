//
// Profiling code helpers.
//
// Usage:
//
//   PROFILE_DECLARE_SECTION(function_outer);
//   PROFILE_DECLARE_SECTION(function_inner);
//
//   void function() {
//     PROFILE_TIME_SECTION(function_outer);
//     for (...) {
//       PROFILE_TIME_SECTION(function_inner);
//     }
//   }
//
//   int main() {
//     for (...) {
//       function();
//       PROFILE_PRINT_TIMINGS();
//     }
//   }
//

#ifndef PROFILE_H
#define PROFILE_H

// Toggle profiling code.
#define PROFILE_ENABLED 0

#if PROFILE_ENABLED

#include "funcs.h"

namespace profiling {

#define PROFILE_DECLARE_SECTION(section) \
    static profiling::ScopeInfo _profinfo_##section = { #section, NULL, 0 }; \
    static UNUSED int _profinfo_register_##section = register_scope( &_profinfo_##section )
#define PROFILE_TIME_SECTION(section) \
    profiling::ProfScope CONCAT(_profscope_, __LINE__) ( &_profinfo_##section )
#define PROFILE_PRINT_TIMINGS() profiling::print()

struct ScopeInfo {
    const char *name;
    ScopeInfo *next;
    uclock_t ticks;
};

FASTCALL int register_scope(ScopeInfo *info);
FASTCALL void print();

static FORCEINLINE void scope_start(ScopeInfo *info) {
    // Timing is end-start, so do the -start here.
    info->ticks -= Funcs98::ticks();
}

static FORCEINLINE void scope_end(ScopeInfo* info) {
    // Add on end to get full timing.
    info->ticks += Funcs98::ticks();
}

// Helper to capture a scope.
struct ProfScope {
    ScopeInfo *info;
    ProfScope(ScopeInfo *i) : info(i) { scope_start(info); }
    ~ProfScope() { scope_end(info); }
};

} // namespace profiling

#else // PROFILE_ENABLED

#define PROFILE_DECLARE_SECTION(section) struct _profinfo_##section {}
#define PROFILE_TIME_SECTION(section) do { } while(0)
#define PROFILE_PRINT_TIMINGS() do { } while(0)

#endif // PROFILE_ENABLED

#endif
